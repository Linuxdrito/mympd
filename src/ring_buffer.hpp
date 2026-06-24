#pragma once
#include <vector>
#include <atomic>
#include <algorithm>

class AudioRingBuffer {
public:
    AudioRingBuffer(size_t capacity) : buffer_(capacity), capacity_(capacity) {}

    size_t write(const float* data, size_t count) {
        size_t w = write_pos_.load(std::memory_order_relaxed);
        size_t r = read_pos_.load(std::memory_order_acquire);
        size_t available = capacity_ - 1 - (w - r + capacity_) % capacity_;
        size_t to_write = std::min(count, available);

        for (size_t i = 0; i < to_write; ++i) {
            buffer_[(w + i) % capacity_] = data[i];
        }
        write_pos_.store((w + to_write) % capacity_, std::memory_order_release);
        return to_write;
    }

    size_t read(float* data, size_t count) {
        // Consumidor intercepta la petición de flush
        if (flush_request_.exchange(false, std::memory_order_acquire)) {
            read_pos_.store(0, std::memory_order_relaxed);
            write_pos_.store(0, std::memory_order_release); // Release para el productor
            return 0; 
        }

        size_t r = read_pos_.load(std::memory_order_relaxed);
        size_t w = write_pos_.load(std::memory_order_acquire);
        size_t available = (w - r + capacity_) % capacity_;
        size_t to_read = std::min(count, available);

        for (size_t i = 0; i < to_read; ++i) {
            data[i] = buffer_[(r + i) % capacity_];
        }
        read_pos_.store((r + to_read) % capacity_, std::memory_order_release);
        return to_read;
    }

    void request_flush() {
        flush_request_.store(true, std::memory_order_release);
    }

    size_t available_read() const {
        size_t w = write_pos_.load(std::memory_order_acquire);
        size_t r = read_pos_.load(std::memory_order_acquire);
        return (w - r + capacity_) % capacity_;
    }

private:
    std::vector<float> buffer_;
    size_t capacity_;
    std::atomic<size_t> read_pos_{0};
    std::atomic<size_t> write_pos_{0};
    std::atomic<bool> flush_request_{false};
};
