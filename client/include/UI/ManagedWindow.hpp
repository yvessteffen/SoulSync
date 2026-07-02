#pragma once
#include <SDL3/SDL.h>
struct ManagedWindow
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};