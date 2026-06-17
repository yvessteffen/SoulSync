add_executable(SoulSyncClient
    src/main.cpp
    src/Emulator.cpp
)

target_include_directories(SoulSyncClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SoulSyncClient PRIVATE
    mgba
    Shlwapi
    ws2_32
)

target_compile_definitions(SoulSyncClient PRIVATE
    ENABLE_VFS
    ENABLE_DIRECTORIES
    ENABLE_VFS_FD
    M_CORE_GBA
)