#pragma once
#include <vector>
#include <string>

class Playlist {
public:
    Playlist(const std::string& directory);
    void scan();
    std::string current() const;
    std::string next();
    std::string prev();
    bool empty() const;
    size_t size() const;

private:
    std::string directory_;
    std::vector<std::string> tracks_;
    size_t current_index_{0};
};
