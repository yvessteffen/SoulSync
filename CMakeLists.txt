cmake_minimum_required(VERSION 3.20)

project(SoulSync VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SoulSync
    app/main.cpp
)