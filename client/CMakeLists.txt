find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/external/ffmpeg)
set(FFMPEG_INCLUDE_DIR ${FFMPEG_DIR}/include)
set(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib)

add_executable(SoulSyncClient
    src/main.cpp

    src/Emulator/Emulator.cpp
    src/Emulator/config.cpp

    src/Video/Capture.cpp
    src/Video/Encoder.cpp
    src/Video/Decoder.cpp

    src/UI/WindowManager.cpp
)

target_include_directories(SoulSyncClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/common
)

target_link_libraries(SoulSyncClient PRIVATE
    mgba
    Shlwapi
    ws2_32
    SDL3::SDL3
    imgui::imgui
    ${FFMPEG_LIB_DIR}/avcodec.lib
    ${FFMPEG_LIB_DIR}/avformat.lib
    ${FFMPEG_LIB_DIR}/avutil.lib
    ${FFMPEG_LIB_DIR}/swscale.lib
    ${FFMPEG_LIB_DIR}/swresample.lib
)

target_compile_definitions(SoulSyncClient PRIVATE
    SDL_MAIN_HANDLED
    ENABLE_VFS
    ENABLE_DIRECTORIES
    ENABLE_VFS_FD
    M_CORE_GBA
)

file(GLOB FFMPEG_DLLS "${FFMPEG_DIR}/bin/*.dll")
foreach(DLL ${FFMPEG_DLLS})
    add_custom_command(TARGET SoulSyncClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL}
        $<TARGET_FILE_DIR:SoulSyncClient>
    )
endforeach()