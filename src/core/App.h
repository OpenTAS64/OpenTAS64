#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "tas/TasProject.h"

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
    void RenderInputEditor();
    void RenderTimeline();
    void SaveProject();
    void LoadProject();
    void CreateNewProject();

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TasProject project;
    char projectPath[512] = "project.otas";
    std::string fileStatus = "No project file loaded.";
    bool fileStatusOk = true;
    bool running = true;
};
