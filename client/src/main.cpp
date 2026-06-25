#include <iostream>
#include "Emulator/Emulator.hpp"
#include "Video/Capture.hpp"
#include "Video/Framequeue.hpp"
#include "Video/Encoder.hpp"
#include <chrono>
#include <thread>
#include <SDL3/SDL.h>
#include "Emulator/config.hpp"
#include "filesystem"

using namespace std::chrono;

constexpr auto FRAME_DURATION = duration<double>(1.0 / 59.7275);
const int width = 240;
const int height = 160;

#ifndef NDEBUG
#define DEBUG_LOG(x) std::cerr << x << '\n'
#else
#define DEBUG_LOG(x) ((void)0)
#endif

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);

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

    Emulator emulator;
    if (!emulator.initialize()) {
        DEBUG_LOG("init failed");
        return 1;
    }
    DEBUG_LOG("Emulator initialized\n");

    if (!emulator.loadRom(rompath)) {
        DEBUG_LOG("Failed to load ROM\n");
        return 1;
    }
    DEBUG_LOG("Rom loaded\n");

    if (!emulator.loadSave(savepath.c_str())) {
        DEBUG_LOG("Failed to load Savegame\n");
        return 1;
    }
    #ifndef NDEBUG
    DEBUG_LOG("Save loaded\n");
    #endif

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
                    DEBUG_LOG("Controller: " << SDL_GetGamepadName(controller) << "\n");
                break;
            }
        }
    }

    //SDL_CreateWindowAndRenderer();
    SDL_Window* emuWindow = SDL_CreateWindow("SoulLink GBA", width * 3, height * 3, SDL_WINDOW_RESIZABLE);
    SDL_Window* stream1Window = SDL_CreateWindow("Stream 1", width * 3, height * 3, SDL_WINDOW_RESIZABLE);
    SDL_Window* stream2Window = SDL_CreateWindow("Stream 2", width * 3, height * 3, SDL_WINDOW_RESIZABLE);

    SDL_Renderer* emuRenderer     = SDL_CreateRenderer(emuWindow,     nullptr);
    SDL_Renderer* stream1Renderer = SDL_CreateRenderer(stream1Window, nullptr);
    SDL_Renderer* stream2Renderer = SDL_CreateRenderer(stream2Window, nullptr);

    SDL_Texture* emuTexture = SDL_CreateTexture(
        emuRenderer, SDL_PIXELFORMAT_XBGR8888,
        SDL_TEXTUREACCESS_STREAMING, width, height
    );
    SDL_SetTextureScaleMode(emuTexture, SDL_SCALEMODE_NEAREST);

    SDL_Texture* stream1Texture = SDL_CreateTexture(
        stream1Renderer, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING, width, height
    );
    SDL_SetTextureScaleMode(stream1Texture, SDL_SCALEMODE_NEAREST);

    SDL_Texture* stream2Texture = SDL_CreateTexture(
        stream2Renderer, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING, width, height
    );
    SDL_SetTextureScaleMode(stream2Texture, SDL_SCALEMODE_NEAREST);

    emulator.start();
    if (!emulator.initAudio(48000)) {
        DEBUG_LOG("Failed to init audio\n");
        return 1;
    }
    DEBUG_LOG("Emulator started\n");

    bool running = true;
    bool turboActive = false;
    bool triggerWasPressed = false;

    Capture capture(256, height);
    FrameQueue<Frame> frameQueue;
    uint64_t frameNumber = 0;
    int frame = 0;
    Encoder encoder;
    std::string recordPath = basePath + "data/recordings/output.mkv";
    // create recordings dir if needed
    std::filesystem::create_directories(basePath + "data/recordings");
    encoder.open(recordPath, width, height, streamingFps);

    while (running)
    {
        auto start = steady_clock::now();

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
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
        emulator.runFrame();
        emulator.processAudio();

        const Framebuffer& fb = emulator.getFramebuffer();
        frame++;

        if(!turboActive && (frame % (60/streamingFps) == 0))
        {
            Frame videoframe = capture.capture(
                static_cast<const uint32_t*>(fb.pixels),
                frameNumber++
            );
            encoder.encodeFrame(videoframe.pixels.data(), videoframe.width, videoframe.height);
            frameQueue.push(std::move(videoframe));
            frame = 0;
        } else {
            if(turboActive && (frame % (60/streamingFps*int(turboSpeed)) == 0)){
                Frame videoframe = capture.capture(
                    static_cast<const uint32_t*>(fb.pixels),
                    frameNumber++
                );
                encoder.encodeFrame(videoframe.pixels.data(), videoframe.width, videoframe.height);
                frameQueue.push(std::move(videoframe));
                frame = 0;
            }
        }

        SDL_UpdateTexture(emuTexture, nullptr, fb.pixels, 256 * 4);
    
        SDL_RenderClear(emuRenderer);
        SDL_RenderClear(stream1Renderer);
        SDL_RenderClear(stream2Renderer);

        SDL_RenderTexture(emuRenderer, emuTexture, NULL, NULL);
        SDL_RenderTexture(stream1Renderer, stream1Texture, NULL, NULL);
        SDL_RenderTexture(stream2Renderer, stream2Texture, NULL, NULL);

        SDL_RenderPresent(emuRenderer);
        SDL_RenderPresent(stream1Renderer);
        SDL_RenderPresent(stream2Renderer);

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

    SDL_Quit();
    return 0;
}