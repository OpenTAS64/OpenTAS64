#pragma once
#include <SDL2/SDL.h>
#include <string>
#include "tas/TasProject.h"
class App {
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
    SDL_Window* window=nullptr;
    SDL_Renderer* renderer=nullptr;
    TasProject project;
    char projectPath[256]="project.otas";
    std::string fileStatus;
    bool fileStatusOk=false;
    bool running=true;
};
