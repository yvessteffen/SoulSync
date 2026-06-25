find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(SoulSyncClient
    src/main.cpp
    src/Core/Emulator.cpp
    src/config.cpp
)

target_include_directories(SoulSyncClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SoulSyncClient PRIVATE
    mgba
    Shlwapi
    ws2_32
    SDL3::SDL3
    imgui::imgui
)

target_compile_definitions(SoulSyncClient PRIVATE
    SDL_MAIN_HANDLED
    ENABLE_VFS
    ENABLE_DIRECTORIES
    ENABLE_VFS_FD
    M_CORE_GBA
)