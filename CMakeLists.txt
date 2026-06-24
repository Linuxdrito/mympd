cmake_minimum_required(VERSION 3.20)
project(mympd VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimizaciones agresivas
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -flto -pipe")

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswresample)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

add_executable(mympd
    src/main.cpp
    src/player.cpp
    src/playlist.cpp
    src/pipewire_output.cpp
    src/command_server.cpp
)
target_link_libraries(mympd PkgConfig::FFMPEG PkgConfig::PIPEWIRE pthread)

add_executable(mympc
    src/cli.cpp
)
