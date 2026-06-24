#include "command_server.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

CommandServer::CommandServer(const std::string& socket_path, CommandHandler handler)
    : socket_path_(socket_path), handler_(handler) {}

CommandServer::~CommandServer() { stop(); }

void CommandServer::start() {
    running_ = true;
    server_thread_ = std::thread(&CommandServer::run, this);
}

void CommandServer::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) server_thread_.join();
    unlink(socket_path_.c_str());
}

void CommandServer::run() {
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
    unlink(socket_path_.c_str());

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) return;
    if (listen(server_fd_, 5) < 0) return;

    while (running_) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) continue;

        char buffer[256]{0};
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            std::string cmd(buffer);
            cmd.erase(cmd.find_last_not_of(" \n\r\t") + 1);
            
            std::string response = handler_(cmd) + "\n";
            write(client_fd, response.c_str(), response.length());
        }
        close(client_fd);
    }
}
