cmake_minimum_required(VERSION 3.9)

project(F4MP VERSION 0.2
    DESCRIPTION "Fallout 4 Multiplayer Mod"
    LANGUAGES CXX)

add_subdirectory(./client)
add_subdirectory(./server)