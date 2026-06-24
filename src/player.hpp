#pragma once
#include "playlist.hpp"
#include "pipewire_output.hpp"
#include "ring_buffer.hpp"
#include <atomic>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

class Player {
public:
    Player(Playlist& playlist);
    ~Player();

    void play();
    void pause();
    void stop();
    void next();
    void prev();
    void seek(int seconds);
    std::string status() const;

private:
    void audio_callback(float* buffer, uint32_t frames);
    void decoder_loop();
    bool open_file(const std::string& path);
    void close_file();

    Playlist& playlist_;
    PipewireOutput pw_out_;
    
    enum class State { STOPPED, PLAYING, PAUSED };
    std::atomic<State> state_{State::STOPPED};

    enum class Command { NONE, PLAY, PAUSE, STOP, NEXT, PREV, SEEK };
    std::atomic<Command> pending_command_{Command::NONE};
    std::atomic<int> seek_offset_{0};

    std::thread decoder_thread_;
    std::atomic<bool> quit_{false};

    // Buffer de 2.5 segundos para 48kHz stereo (48000 * 2 canales * 2.5 seg)
    AudioRingBuffer ring_buffer_{240000}; 
    std::vector<float> resample_buffer_;

    AVFormatContext* format_ctx_{nullptr};
    AVCodecContext* codec_ctx_{nullptr};
    SwrContext* swr_ctx_{nullptr};
    AVPacket* packet_{nullptr};
    AVFrame* frame_{nullptr};
    int audio_stream_idx_{-1};
    int64_t current_pts_{0};
};
