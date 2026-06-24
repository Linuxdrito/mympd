#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>

class CommandServer {
public:
    using CommandHandler = std::function<std::string(const std::string&)>;
    CommandServer(const std::string& socket_path, CommandHandler handler);
    ~CommandServer();
    void start();
    void stop();

private:
    void run();
    std::string socket_path_;
    CommandHandler handler_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    int server_fd_{-1};
};
