#include <iostream>
#include "Emulator.hpp"
#include <chrono>
#include <thread>
#include <SDL.h>

using namespace std::chrono;
constexpr auto FRAME_DURATION = duration<double>(1.0 / 59.7275);

int main()
{
    const int width = 240;
    const int height = 160;

    std::cout << "SoulSync Client\n";

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);

    Emulator emulator;

    if (!emulator.initialize()) {
        std::cout << "init failed\n";
        return 1;
    }
    std::cout << "Emulator initialized\n";

    const char* path = "C:\\Users\\yvess\\Desktop\\Private Projects\\SoulSync\\build\\client\\Release\\rom.gba";    
    if (!emulator.loadRom(path)) {
        std::cerr << "Failed to load ROM\n";
        return 1;
    }
    std::cout << "Rom loaded\n";

    emulator.start();
    std::cout << "Emulator started\n";

    SDL_GameController* controller = nullptr;
    

    SDL_Window* window = SDL_CreateWindow(
        "SoulLink GBA",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width * 3,
        height * 3,
        SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    bool running = true;

    while (running) 
    {
        SDL_Event e;

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;
        }

        auto start = steady_clock::now();
        emulator.runFrame();

        Framebuffer fb = emulator.getFramebuffer();

        SDL_UpdateTexture(
            texture,
            NULL,
            fb.pixels,
            256 * 4
        );

        SDL_RenderClear(renderer);
        SDL_RenderCopy(
            renderer,
            texture,
            NULL,
            NULL
        );

        SDL_RenderPresent(renderer);

        auto elapsed = steady_clock::now() - start;
        std::this_thread::sleep_for(FRAME_DURATION - elapsed);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}