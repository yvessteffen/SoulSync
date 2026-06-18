find_package(SDL2 CONFIG REQUIRED)

add_executable(SoulSyncClient
    src/main.cpp
    src/Core/Emulator.cpp
)

target_include_directories(SoulSyncClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SoulSyncClient PRIVATE
    mgba
    Shlwapi
    ws2_32
    SDL2::SDL2
)

target_compile_definitions(SoulSyncClient PRIVATE
    SDL_MAIN_HANDLED
    ENABLE_VFS
    ENABLE_DIRECTORIES
    ENABLE_VFS_FD
    M_CORE_GBA
)