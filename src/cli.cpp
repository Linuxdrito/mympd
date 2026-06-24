#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: mympc <command>\n";
        return 1;
    }

    std::string command = argv[1];
    for (int i = 2; i < argc; ++i) {
        command += " ";
        command += argv[i];
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return 1;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/mympd.sock", sizeof(addr.sun_path) - 1);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "mympd daemon inactivo.\n";
        close(sock);
        return 1;
    }

    write(sock, command.c_str(), command.length());

    char buffer[256]{0};
    if (read(sock, buffer, sizeof(buffer) - 1) > 0) {
        std::cout << buffer;
    }

    close(sock);
    return 0;
}
