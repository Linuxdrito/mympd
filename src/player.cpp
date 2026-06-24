#include "player.hpp"
#include <iostream>

Player::Player(Playlist& playlist) : playlist_(playlist) {
    packet_ = av_packet_alloc();
    frame_ = av_frame_alloc();
    resample_buffer_.resize(48000 * 2); // Buffer para 1 segundo max por frame (sobrado)
    
    pw_out_.init([this](float* buf, uint32_t f) { audio_callback(buf, f); });
    decoder_thread_ = std::thread(&Player::decoder_loop, this);
}

Player::~Player() {
    quit_ = true;
    if (decoder_thread_.joinable()) decoder_thread_.join();
    close_file();
    av_frame_free(&frame_);
    av_packet_free(&packet_);
}

void Player::play() { pending_command_ = Command::PLAY; }
void Player::pause() { pending_command_ = Command::PAUSE; }
void Player::stop() { pending_command_ = Command::STOP; }
void Player::next() { pending_command_ = Command::NEXT; }
void Player::prev() { pending_command_ = Command::PREV; }
void Player::seek(int seconds) { 
    seek_offset_ = seconds; 
    pending_command_ = Command::SEEK; 
}

std::string Player::status() const {
    State s = state_.load();
    std::string str = (s == State::PLAYING) ? "Playing" : (s == State::PAUSED ? "Paused" : "Stopped");
    if (s != State::STOPPED) str += ": " + playlist_.current();
    return str;
}

bool Player::open_file(const std::string& path) {
    close_file();
    if (avformat_open_input(&format_ctx_, path.c_str(), nullptr, nullptr) < 0) return false;
    if (avformat_find_stream_info(format_ctx_, nullptr) < 0) return false;

    const AVCodec* codec = nullptr;
    audio_stream_idx_ = av_find_best_stream(format_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (audio_stream_idx_ < 0) return false;

    codec_ctx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx_, format_ctx_->streams[audio_stream_idx_]->codecpar);
    avcodec_open2(codec_ctx_, codec, nullptr);

    av_channel_layout_default(&codec_ctx_->ch_layout, 2);
    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, 2);

    swr_alloc_set_opts2(&swr_ctx_,
                        &out_ch_layout, AV_SAMPLE_FMT_FLT, 48000,
                        &codec_ctx_->ch_layout, codec_ctx_->sample_fmt, codec_ctx_->sample_rate,
                        0, nullptr);
    swr_init(swr_ctx_);
    current_pts_ = 0;
    return true;
}

void Player::close_file() {
    if (swr_ctx_) swr_free(&swr_ctx_);
    if (codec_ctx_) avcodec_free_context(&codec_ctx_);
    if (format_ctx_) avformat_close_input(&format_ctx_);
    audio_stream_idx_ = -1;
}

void Player::decoder_loop() {
    while (!quit_) {
        Command cmd = pending_command_.exchange(Command::NONE);
        
        if (cmd == Command::PLAY && state_ != State::PLAYING) {
            if (state_ == State::STOPPED && !playlist_.empty()) {
                open_file(playlist_.current());
            }
            state_ = State::PLAYING;
        } 
        else if (cmd == Command::PAUSE) { state_ = State::PAUSED; } 
        else if (cmd == Command::STOP) { 
            state_ = State::STOPPED;
            close_file();
            ring_buffer_.request_flush();
        } 
        else if (cmd == Command::NEXT) {
            open_file(playlist_.next());
            ring_buffer_.request_flush();
            if (codec_ctx_) avcodec_flush_buffers(codec_ctx_);
            state_ = State::PLAYING;
        } 
        else if (cmd == Command::PREV) {
            open_file(playlist_.prev());
            ring_buffer_.request_flush();
            if (codec_ctx_) avcodec_flush_buffers(codec_ctx_);
            state_ = State::PLAYING;
        } 
        else if (cmd == Command::SEEK && format_ctx_) {
            AVStream* stream = format_ctx_->streams[audio_stream_idx_];
            int64_t offset = (seek_offset_.load() * stream->time_base.den) / stream->time_base.num;
            int64_t target = current_pts_ + offset;
            if (target < 0) target = 0;
            
            av_seek_frame(format_ctx_, audio_stream_idx_, target, AVSEEK_FLAG_ANY);
            ring_buffer_.request_flush();
            avcodec_flush_buffers(codec_ctx_);
        }

        if (state_ != State::PLAYING || !format_ctx_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (ring_buffer_.available_read() < 144000) { // Llenar si baja de ~1.5 segundos
            if (av_read_frame(format_ctx_, packet_) >= 0) {
                if (packet_->stream_index == audio_stream_idx_) {
                    avcodec_send_packet(codec_ctx_, packet_);
                    while (avcodec_receive_frame(codec_ctx_, frame_) == 0) {
                        current_pts_ = frame_->best_effort_timestamp;
                        
                        uint8_t* out_ptrs[1] = { reinterpret_cast<uint8_t*>(resample_buffer_.data()) };
                        int out_samples = swr_convert(swr_ctx_, out_ptrs, resample_buffer_.size() / 2,
                                                      (const uint8_t**)frame_->data, frame_->nb_samples);
                        
                        if (out_samples > 0) {
                            ring_buffer_.write(resample_buffer_.data(), out_samples * 2);
                        }
                    }
                }
                av_packet_unref(packet_);
            } else {
                // EOF seguro
                pending_command_ = Command::NEXT;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void Player::audio_callback(float* buffer, uint32_t frames) {
    uint32_t samples_needed = frames * 2;
    
    if (state_.load() != State::PLAYING) {
        std::fill(buffer, buffer + samples_needed, 0.0f);
        return;
    }

    size_t read = ring_buffer_.read(buffer, samples_needed);
    for (size_t i = read; i < samples_needed; ++i) {
        buffer[i] = 0.0f;
    }
}
