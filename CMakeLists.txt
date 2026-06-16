cmake_minimum_required(VERSION 3.20)
project(SoulSync LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mgba STATIC IMPORTED)
set_target_properties(mgba PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/mgba/build/Release/mgba.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/mgba/include"
)

add_subdirectory(client)