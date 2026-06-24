#pragma once
#include <pipewire/pipewire.h>
#include <functional>
#include <thread>
#include <atomic>

class PipewireOutput {
public:
    using AudioCallback = std::function<void(float* buffer, uint32_t frames)>;

    PipewireOutput();
    ~PipewireOutput();

    bool init(AudioCallback callback);

private:
    static void on_process(void *userdata);

    AudioCallback callback_;
    pw_main_loop *loop_{nullptr};
    pw_context *context_{nullptr};
    pw_core *core_{nullptr};
    pw_stream *stream_{nullptr};
    std::thread pw_thread_;
};
