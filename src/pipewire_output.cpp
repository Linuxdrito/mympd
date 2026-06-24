#include "pipewire_output.hpp"
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>
#include <iostream>

PipewireOutput::PipewireOutput() { pw_init(nullptr, nullptr); }

PipewireOutput::~PipewireOutput() {
    if (loop_) pw_main_loop_quit(loop_);
    if (pw_thread_.joinable()) pw_thread_.join();
    if (stream_) pw_stream_destroy(stream_);
    if (core_) pw_core_disconnect(core_);
    if (context_) pw_context_destroy(context_);
    if (loop_) pw_main_loop_destroy(loop_);
    pw_deinit();
}

void PipewireOutput::on_process(void *userdata) {
    auto *pw_out = static_cast<PipewireOutput*>(userdata);
    pw_buffer *b = pw_stream_dequeue_buffer(pw_out->stream_);
    if (!b) return;

    spa_buffer *buf = b->buffer;
    float *dst = static_cast<float*>(buf->datas[0].data);
    if (!dst) return;

    uint32_t stride = sizeof(float) * 2; 
    uint32_t n_frames = buf->datas[0].maxsize / stride;
    if (b->requested) n_frames = std::min(n_frames, (uint32_t)b->requested);

    // Llamada directa y sin locks
    pw_out->callback_(dst, n_frames);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;
    pw_stream_queue_buffer(pw_out->stream_, b);
}

bool PipewireOutput::init(AudioCallback callback) {
    callback_ = callback;
    loop_ = pw_main_loop_new(nullptr);
    context_ = pw_context_new(pw_main_loop_get_loop(loop_), nullptr, 0);
    core_ = pw_context_connect(context_, nullptr, 0);

    const pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .process = on_process
    };

    stream_ = pw_stream_new_simple(
        pw_main_loop_get_loop(loop_),
        "mympd_stream",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            nullptr),
        &stream_events,
        this);

    uint8_t buffer[1024];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const spa_pod *params[1];
    
    spa_audio_info_raw info = {
        .format = SPA_AUDIO_FORMAT_F32,
        .rate = 48000,
        .channels = 2
    };
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    pw_stream_connect(stream_, PW_DIRECTION_OUTPUT, PW_ID_ANY,
                      (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                      PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS),
                      params, 1);

    pw_thread_ = std::thread([this]() { pw_main_loop_run(loop_); });
    return true;
}
