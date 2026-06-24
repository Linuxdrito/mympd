#include "playlist.hpp"
#include <filesystem>
#include <algorithm>
#include <cstdlib>

namespace fs = std::filesystem;

Playlist::Playlist(const std::string& directory) {
    if (directory.starts_with("~/")) {
        const char* home = std::getenv("HOME");
        directory_ = home ? std::string(home) + directory.substr(1) : directory;
    } else {
        directory_ = directory;
    }
}

void Playlist::scan() {
    tracks_.clear();
    if (!fs::exists(directory_)) return;

    for (const auto& entry : fs::recursive_directory_iterator(directory_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mp3") {
            tracks_.push_back(entry.path().string());
        }
    }
    std::sort(tracks_.begin(), tracks_.end());
    current_index_ = 0;
}

std::string Playlist::current() const { return tracks_.empty() ? "" : tracks_[current_index_]; }
std::string Playlist::next() {
    if (tracks_.empty()) return "";
    current_index_ = (current_index_ + 1) % tracks_.size();
    return tracks_[current_index_];
}
std::string Playlist::prev() {
    if (tracks_.empty()) return "";
    current_index_ = (current_index_ == 0) ? tracks_.size() - 1 : current_index_ - 1;
    return tracks_[current_index_];
}
bool Playlist::empty() const { return tracks_.empty(); }
size_t Playlist::size() const { return tracks_.size(); }
