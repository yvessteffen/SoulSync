#include "Emulator/Emulator.hpp"
#include "Emulator/config.hpp"
#include "Video/Capture.hpp"
#include "Video/Framequeue.hpp"
#include "Video/Encoder.hpp"
#include "Video/Decoder.hpp"
#include "Network/ReceiveState.hpp"
#include "util/logger.h"

#include <SDL3/SDL.h>
#include "imgui.h"
#include <imgui_impl_sdl3.h>
#include "imgui_impl_sdlrenderer3.h"

#include <iostream>
#include <chrono>
#include <thread>
#include "filesystem"
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std::chrono;

constexpr auto FRAME_DURATION = duration<double>(1.0 / 59.7275);
const int width = 240;
const int height = 160;

static SDL_FRect CalculateAspectRect(
    int contentW, int contentH,
    float offsetX, float offsetY,
    int availW, int availH
) {
    float targetAspect = (float)contentW / (float)contentH;
    float windowAspect = (float)availW / (float)availH;

    SDL_FRect rect;

    if (windowAspect > targetAspect) {
        rect.h = (float)availH;
        rect.w = rect.h * targetAspect;

        rect.x = offsetX + (availW - rect.w) * 0.5f;
        rect.y = offsetY;
    } else {
        rect.w = (float)availW;
        rect.h = rect.w / targetAspect;

        rect.x = offsetX;
        rect.y = offsetY + (availH - rect.h) * 0.5f;
    }

    return rect;
}

static void MainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("SoulSync"))
        {
            if (ImGui::MenuItem("Reset")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Backup Savefile")) {}
            if (ImGui::MenuItem("Randomize and Reset")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) {}
            ImGui::EndMenu();
        }
        
        if (ImGui::Button("Settings")) {
            static float v = 100.0f;
            static float s = 3.0f;
            ImGui::SliderFloat("Volume", &v, 0.0f, 100.0f);
            ImGui::SliderFloat("Speed Up X", &s, 0.0f, 10.0f);
        }

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::EndMainMenuBar();
    }
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
 
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    std::string basePath = SDL_GetBasePath();
    std::string rompath = basePath + "/data/rom/" + "rom.gba";
    std::string randomizedrompath = basePath + "/data/randomizedrom/" + "randomized.gba";
    std::string randomizerpath = basePath + "/data/randomizer/" + "randomizer.jar";
    std::string settingspath = basePath + "/data/settings/" + "settings.rnqs";
    std::string savepath = basePath + "/data/randomizedrom/" + "randomized.sav";
    std::string backuppath = basePath + "/data/backupsave/";
    std::string inipath = basePath + "/soulsync.ini";

    Config config;
    config.load(inipath);

    auto getScancode = [](const std::string& name) -> SDL_Scancode {
        return SDL_GetScancodeFromName(name.c_str());
    };

    auto getGamepadButton = [](const std::string& name) -> SDL_GamepadButton {
        if (name == "South")         return SDL_GAMEPAD_BUTTON_SOUTH;
        if (name == "East")          return SDL_GAMEPAD_BUTTON_EAST;
        if (name == "West")          return SDL_GAMEPAD_BUTTON_WEST;
        if (name == "North")         return SDL_GAMEPAD_BUTTON_NORTH;
        if (name == "Start")         return SDL_GAMEPAD_BUTTON_START;
        if (name == "Back")          return SDL_GAMEPAD_BUTTON_BACK;
        if (name == "DpadUp")        return SDL_GAMEPAD_BUTTON_DPAD_UP;
        if (name == "DpadDown")      return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
        if (name == "DpadLeft")      return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
        if (name == "DpadRight")     return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
        if (name == "LeftShoulder")  return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
        if (name == "RightShoulder") return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
        if (name == "LeftStick")     return SDL_GAMEPAD_BUTTON_LEFT_STICK;
        if (name == "RightStick")    return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        return SDL_GAMEPAD_BUTTON_INVALID;
    };

    // keyboard bindings
    SDL_Scancode key_a      = getScancode(config.get("controls", "a",      "X"));
    SDL_Scancode key_b      = getScancode(config.get("controls", "b",      "Z"));
    SDL_Scancode key_start  = getScancode(config.get("controls", "start",  "Return"));
    SDL_Scancode key_select = getScancode(config.get("controls", "select", "Backspace"));
    SDL_Scancode key_up     = getScancode(config.get("controls", "up",     "Up"));
    SDL_Scancode key_down   = getScancode(config.get("controls", "down",   "Down"));
    SDL_Scancode key_left   = getScancode(config.get("controls", "left",   "Left"));
    SDL_Scancode key_right  = getScancode(config.get("controls", "right",  "Right"));
    SDL_Scancode key_l      = getScancode(config.get("controls", "l",      "A"));
    SDL_Scancode key_r      = getScancode(config.get("controls", "r",      "S"));

    // controller bindings
    SDL_GamepadButton btn_a      = getGamepadButton(config.get("controller", "a",      "South"));
    SDL_GamepadButton btn_b      = getGamepadButton(config.get("controller", "b",      "East"));
    SDL_GamepadButton btn_start  = getGamepadButton(config.get("controller", "start",  "Start"));
    SDL_GamepadButton btn_select = getGamepadButton(config.get("controller", "select", "Back"));
    SDL_GamepadButton btn_up     = getGamepadButton(config.get("controller", "up",     "DpadUp"));
    SDL_GamepadButton btn_down   = getGamepadButton(config.get("controller", "down",   "DpadDown"));
    SDL_GamepadButton btn_left   = getGamepadButton(config.get("controller", "left",   "DpadLeft"));
    SDL_GamepadButton btn_right  = getGamepadButton(config.get("controller", "right",  "DpadRight"));
    SDL_GamepadButton btn_l      = getGamepadButton(config.get("controller", "l",      "LeftShoulder"));
    SDL_GamepadButton btn_r      = getGamepadButton(config.get("controller", "r",      "RightShoulder"));

    int16_t deadzone = (int16_t)std::stoi(config.get("controller_axis", "deadzone", "8000"));
    int16_t triggerThreshold = (int16_t)std::stoi(config.get("controller_axis", "right_trigger_threshold", "16000"));

    float turboSpeed = std::stof(config.get("emulator", "turbo_speed", "3.0"));
    auto FRAME_DURATION_TURBO = duration<double>(1.0 / (59.7275 * turboSpeed));
    int streamingFps = std::stoi(config.get("streaming", "fps", "30"));

    auto IP = config.get("streaming", "serverIP", "");
    auto senderID = config.get("streaming", "name", "NoName");
    PCSTR serverIP = IP.c_str();

    Emulator emulator;
    if (!emulator.initialize()) {
        Log("init failed\n");
        return 1;
    }
    Log("Emulator initialized\n");

    if (!emulator.loadRom(rompath)) {
        Log("Failed to load ROM\n");
        return 1;
    }
    Log("Rom loaded\n");

    if (!emulator.loadSave(savepath.c_str())) {
        Log("Failed to load Savegame\n");
        return 1;
    }
    Log("Save loaded\n");

    SOCKET streamSock = socket(AF_INET, SOCK_STREAM, 0);

    if (streamSock == INVALID_SOCKET)
    {
        Log("Socket creation failed\n");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, serverIP, &addr.sin_addr);

    int result = connect(streamSock, (sockaddr*)&addr, sizeof(addr));

    if (result == SOCKET_ERROR)
    {
        Log("Connect failed: " + std::to_string(WSAGetLastError()) + "\n");
    }
    else
    {
        Log("Connected to relay server\n");
    }

    int count = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);
    SDL_Gamepad* controller = nullptr;
    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            if (SDL_IsGamepad(joysticks[i]))
            {
                controller = SDL_OpenGamepad(joysticks[i]);
                if (controller)
                    Log(std::string("Controller: ") + SDL_GetGamepadName(controller) + "\n");
                break;
            }
        }
    }

    //SDL_CreateWindowAndRenderer();
    std::string emuWindowName = std::string(senderID) + " SoulSync GBA";
    SDL_Window* emuWindow = SDL_CreateWindow(emuWindowName.c_str(), width * 4, height * 4, SDL_WINDOW_RESIZABLE);
    SDL_Window* stream1Window = nullptr;
    SDL_Window* stream2Window = nullptr;

    SDL_Renderer* emuRenderer     = SDL_CreateRenderer(emuWindow,     nullptr);
    SDL_Renderer* stream1Renderer = nullptr;
    SDL_Renderer* stream2Renderer = nullptr;

    SDL_Texture* emuTexture = SDL_CreateTexture(
        emuRenderer, SDL_PIXELFORMAT_XBGR8888,
        SDL_TEXTUREACCESS_STREAMING, width, height
    );
    SDL_SetTextureScaleMode(emuTexture, SDL_SCALEMODE_NEAREST);

    SDL_Texture* stream1Texture = nullptr;
    SDL_Texture* stream2Texture = nullptr;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL3_InitForSDLRenderer(emuWindow, emuRenderer);
    ImGui_ImplSDLRenderer3_Init(emuRenderer);

    emulator.start();
    if (!emulator.initAudio(48000)) {
        Log("Failed to init audio\n");
        return 1;
    }
    else {
        Log("Init audio\n");
    }
    Log("Emulator started\n");

    bool running = true;
    bool turboActive = false;
    bool triggerWasPressed = false;
    bool stream1FirstReceive = true;
    bool stream2FirstReceive= true;
    std::string senderID1;
    std::string senderID2;

    Capture capture(256, height);
    FrameQueue<Frame> frameQueue;
    uint64_t frameNumber = 0;
    int frame = 0;

    Encoder encoder;
    encoder.open(width, height, streamingFps);
    Log("Encoder started\n");

    Decoder stream1Decoder;
    Decoder stream2Decoder;

    stream1Decoder.open();   
    Log("Decoder 1 started\n");
    stream2Decoder.open();
    Log("Decoder 2 started\n");
    ReceiveState recvState;

    std::vector<uint8_t> decodedPixels1;
    int decodedW1 = 0, decodedH1 = 0;
    std::vector<uint8_t> decodedPixels2;
    int decodedW2 = 0, decodedH2 = 0;

    u_long nonBlocking = 1;
    ioctlsocket(streamSock, FIONBIO, &nonBlocking);

    while (running)
    {
        auto start = steady_clock::now();
        Log("Loop started\n");

        SDL_Event e;

        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                SDL_Window* win = SDL_GetWindowFromID(e.window.windowID);
                if (win) {
                    SDL_DestroyWindow(win);
                }
            }
            if (e.type == SDL_EVENT_GAMEPAD_ADDED && controller == nullptr)
                controller = SDL_OpenGamepad(e.cdevice.which);
            if (e.type == SDL_EVENT_GAMEPAD_REMOVED)
            {
                SDL_CloseGamepad(controller);
                controller = nullptr;
            }
        }

        uint32_t keys = 0;
        const bool* state = SDL_GetKeyboardState(nullptr);
        if (state[key_a])      keys |= 0x0001;
        if (state[key_b])      keys |= 0x0002;
        if (state[key_select]) keys |= 0x0004;
        if (state[key_start])  keys |= 0x0008;
        if (state[key_right])  keys |= 0x0010;
        if (state[key_left])   keys |= 0x0020;
        if (state[key_up])     keys |= 0x0040;
        if (state[key_down])   keys |= 0x0080;
        if (state[key_r])      keys |= 0x0100;
        if (state[key_l])      keys |= 0x0200;

        if (controller)
        {
            if (SDL_GetGamepadButton(controller, btn_a))      keys |= 0x0001;
            if (SDL_GetGamepadButton(controller, btn_b))      keys |= 0x0002;
            if (SDL_GetGamepadButton(controller, btn_select)) keys |= 0x0004;
            if (SDL_GetGamepadButton(controller, btn_start))  keys |= 0x0008;
            if (SDL_GetGamepadButton(controller, btn_right))  keys |= 0x0010;
            if (SDL_GetGamepadButton(controller, btn_left))   keys |= 0x0020;
            if (SDL_GetGamepadButton(controller, btn_up))     keys |= 0x0040;
            if (SDL_GetGamepadButton(controller, btn_down))   keys |= 0x0080;
            if (SDL_GetGamepadButton(controller, btn_r))      keys |= 0x0100;
            if (SDL_GetGamepadButton(controller, btn_l))      keys |= 0x0200;

            const int16_t axisX = SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTX);
            const int16_t axisY = SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTY);
            if (axisX >  deadzone) keys |= 0x0010;
            if (axisX < -deadzone) keys |= 0x0020;
            if (axisY < -deadzone) keys |= 0x0040;
            if (axisY >  deadzone) keys |= 0x0080;

            int16_t rightTrigger = SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
            bool triggerPressed = rightTrigger > triggerThreshold;
            if (triggerPressed && !triggerWasPressed)
            {
                turboActive = !turboActive;
                frame = 0;
                SDL_SetAudioStreamFrequencyRatio(
                    emulator.getAudioStream(),
                    turboActive ? turboSpeed : 1.0f
                );
                SDL_ClearAudioStream(emulator.getAudioStream());
            }
            triggerWasPressed = triggerPressed;
        }

        auto frameDuration = turboActive ? FRAME_DURATION_TURBO : FRAME_DURATION;

        emulator.setKeys(keys);
        Log("Emulator set Keys\n");
        emulator.runFrame();
        Log("Emulator Ran Frame\n");
        emulator.processAudio();
        Log("Emulator processed audio\n");

        const Framebuffer& fb = emulator.getFramebuffer();
        frame++;

        std::vector<uint8_t> encodedFrame;

        if(!turboActive && (frame % (60/streamingFps) == 0))
        {
            Frame videoframe = capture.capture(
                static_cast<const uint32_t*>(fb.pixels),
                frameNumber++
            );
            encodedFrame = encoder.encodeFrame(videoframe.pixels.data(),videoframe.width,videoframe.height);
            frameQueue.push(std::move(videoframe));
            frame = 0;
        } else {
            if(turboActive && (frame % (60/streamingFps*int(turboSpeed)) == 0)){
                Frame videoframe = capture.capture(
                    static_cast<const uint32_t*>(fb.pixels),
                    frameNumber++
                );
                encodedFrame = encoder.encodeFrame(videoframe.pixels.data(),videoframe.width,videoframe.height);
                frameQueue.push(std::move(videoframe));
                frame = 0;
            }
        }

        if (!encodedFrame.empty())
        {
            PacketHeader h;
            h.type = 1;
            std::strncpy(h.senderId, senderID.c_str(), sizeof(h.senderId));
            h.senderId[sizeof(h.senderId) - 1] = '\0';
            h.size = (uint32_t)encodedFrame.size();

            send(streamSock, (char*)&h, sizeof(h), 0);
            send(streamSock, (char*)encodedFrame.data(), h.size, 0);
        }

        if (!recvState.headerDone)
        {
            int r = recv(streamSock,
                        (char*)&recvState.header + recvState.headerReceived,
                        sizeof(PacketHeader) - recvState.headerReceived,
                        0);

            if (r > 0)
            {
                recvState.headerReceived += r;

                if (recvState.headerReceived == sizeof(PacketHeader))
                {
                    recvState.headerDone = true;
                    recvState.body.resize(recvState.header.size);
                    recvState.bodyReceived = 0;
                }
            }
        }

        if (recvState.headerDone)
        {
            int r = recv(streamSock,
                        (char*)recvState.body.data() + recvState.bodyReceived,
                        recvState.header.size - recvState.bodyReceived,
                        0);

            if (r > 0)
            {
                recvState.bodyReceived += r;
            }
        }

       if (recvState.headerDone &&
            recvState.bodyReceived == recvState.header.size)
        {
            std::string sender(recvState.header.senderId);
            if (senderID1.empty())
            {
                senderID1 = sender;

                std::string title = sender + " Stream";

                stream1Window = SDL_CreateWindow(
                    title.c_str(),
                    width * 4,
                    height * 4,
                    SDL_WINDOW_RESIZABLE);

                stream1Renderer = SDL_CreateRenderer(stream1Window, nullptr);

                SDL_SetRenderLogicalPresentation(
                    stream1Renderer,
                    width,
                    height,
                    SDL_LOGICAL_PRESENTATION_LETTERBOX
                );

                stream1Texture = SDL_CreateTexture(
                    stream1Renderer,
                    SDL_PIXELFORMAT_RGBA32,
                    SDL_TEXTUREACCESS_STREAMING,
                    width,
                    height);

                SDL_SetTextureScaleMode(stream1Texture, SDL_SCALEMODE_NEAREST);
            }
            else if (sender != senderID1 && senderID2.empty())
            {
                senderID2 = sender;

                std::string title = sender + " Stream";

                stream2Window = SDL_CreateWindow(
                    title.c_str(),
                    width * 3,
                    height * 3,
                    SDL_WINDOW_RESIZABLE
                );

                stream2Renderer = SDL_CreateRenderer(
                    stream2Window, 
                    nullptr
                );

                SDL_SetRenderLogicalPresentation(
                    stream2Renderer,
                    width,
                    height,
                    SDL_LOGICAL_PRESENTATION_LETTERBOX
                );

                stream2Texture = SDL_CreateTexture(
                    stream2Renderer,
                    SDL_PIXELFORMAT_RGBA32,
                    SDL_TEXTUREACCESS_STREAMING,
                    width,
                    height
                );

                SDL_SetTextureScaleMode(
                    stream2Texture, 
                    SDL_SCALEMODE_NEAREST
                );
            }
            if (sender == senderID1)
            {
                if (recvState.header.type == 1)
                {
                    if (stream1Decoder.decodePacket(
                            recvState.body.data(),
                            recvState.body.size(),
                            decodedPixels1,
                            decodedW1,
                            decodedH1))
                    {
                        SDL_UpdateTexture(
                            stream1Texture,
                            nullptr,
                            decodedPixels1.data(),
                            decodedW1 * 4);

                        SDL_RenderClear(stream1Renderer);
                        SDL_RenderTexture(stream1Renderer, stream1Texture, NULL, NULL);
                        SDL_RenderPresent(stream1Renderer);
                    }
                }
            }
            else if (sender == senderID2)
            {
                if (recvState.header.type == 1)
                {
                    if (stream2Decoder.decodePacket(
                            recvState.body.data(),
                            recvState.body.size(),
                            decodedPixels2,
                            decodedW2,
                            decodedH2))
                    {
                        SDL_UpdateTexture(
                            stream2Texture,
                            nullptr,
                            decodedPixels2.data(),
                            decodedW2 * 4);

                        SDL_RenderClear(stream2Renderer);
                        SDL_RenderTexture(stream2Renderer, stream2Texture, NULL, NULL);
                        SDL_RenderPresent(stream2Renderer);
                    }
                }
            }
            recvState = ReceiveState{};
        } 

        int w, h;
        SDL_GetRenderOutputSize(emuRenderer, &w, &h);

        float menuBarHeight = ImGui::GetFrameHeight();

        SDL_FRect dst = CalculateAspectRect(
            240, 160,
            0, menuBarHeight,
            w, h - menuBarHeight
        );

        SDL_UpdateTexture(emuTexture, nullptr, fb.pixels, 256 * 4);
        SDL_RenderClear(emuRenderer);
        SDL_RenderTexture(emuRenderer, emuTexture, nullptr, &dst);

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        MainMenuBar();

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), emuRenderer);

        SDL_RenderPresent(emuRenderer);

        auto elapsed = steady_clock::now() - start;
        double remainingMs = duration<double, std::milli>(frameDuration - elapsed).count();
        if (remainingMs > 0)
            SDL_Delay((Uint32)remainingMs);
    }

    if (controller) SDL_CloseGamepad(controller);

    SDL_DestroyTexture(emuTexture);
    SDL_DestroyTexture(stream1Texture);
    SDL_DestroyTexture(stream2Texture);

    SDL_DestroyRenderer(emuRenderer);
    SDL_DestroyRenderer(stream1Renderer);
    SDL_DestroyRenderer(stream2Renderer);

    SDL_DestroyWindow(emuWindow);
    SDL_DestroyWindow(stream1Window);
    SDL_DestroyWindow(stream2Window);

    encoder.close();

    stream1Decoder.close();
    stream2Decoder.close();

    closesocket(streamSock);

    WSACleanup();
    SDL_Quit();
    return 0;
}