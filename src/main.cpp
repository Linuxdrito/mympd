#include "playlist.hpp"
#include "player.hpp"
#include "command_server.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    keep_running = false;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    Playlist playlist("~/Music");
    playlist.scan();
    
    std::cout << "mympd: " << playlist.size() << " archivos mp3 listos.\n";

    Player player(playlist);

    CommandServer server("/tmp/mympd.sock", [&player](const std::string& cmd) -> std::string {
        if (cmd == "play") { player.play(); return "OK"; }
        if (cmd == "pause") { player.pause(); return "OK"; }
        if (cmd == "stop") { player.stop(); return "OK"; }
        if (cmd == "next") { player.next(); return "OK"; }
        if (cmd == "prev") { player.prev(); return "OK"; }
        if (cmd == "seek +5") { player.seek(5); return "OK"; }
        if (cmd == "seek -5") { player.seek(-5); return "OK"; }
        if (cmd == "status") { return player.status(); }
        return "Command not recognized";
    });

    server.start();

    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    return 0;
}
