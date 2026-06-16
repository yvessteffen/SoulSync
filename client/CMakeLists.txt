add_executable(SoulSyncClient
    src/main.cpp
    src/Emulator.cpp
)

target_include_directories(SoulSyncClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_compile_definitions(SoulSyncClient PRIVATE ENABLE_VFS)

target_link_libraries(SoulSyncClient PRIVATE
    mgba
    ${CMAKE_SOURCE_DIR}/external/mgba/build/libpng/Release/libpng16_static.lib
    ${CMAKE_SOURCE_DIR}/external/mgba/build/zlib/Release/zlibstatic.lib
    Shlwapi
    ws2_32
)