#pragma once

#include <SDL2/SDL.h>

class App
{
public:
    bool Init();
    void Run();
    void Shutdown();

private:
    void HandleEvents();
    void Update();
    void Render();

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    bool running = true;
};