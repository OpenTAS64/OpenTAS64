#include "App.h"
#include <SDL2/SDL.h>
#include <iostream>

SDL_Window* window = nullptr;

bool App::Init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(
        "OpenTAS64",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN
    );

    if (!window)
    {
        std::cerr << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

void App::Run()
{
    while (running)
    {
        HandleEvents();
        Update();
        Render();
    }
}

void App::HandleEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            running = false;
        }
    }
}

void App::Update()
{
    // Später:
    // Emulator aktualisieren
    // Controller lesen
}

void App::Render()
{
    SDL_Delay(16);
}

void App::Shutdown()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}