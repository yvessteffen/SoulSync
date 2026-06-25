cmake_minimum_required(VERSION 3.20)

project(SoulSyncServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SoulSyncServer
    relay_server.cpp
)

if (WIN32)
    target_link_libraries(SoulSyncServer PRIVATE ws2_32)
endif()