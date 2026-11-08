#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <string>
#include "network/TcpClient.h"
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
    void ConnectEmulatorPlugin();

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TasProject project;
    TcpClient connection;
    char projectPath[512] = "project.otas";
    std::string fileStatus = "No project file loaded.";
    std::string connectionStatus = "Not connected";
    bool fileStatusOk = true;
    bool running = true;
    bool playback = false;
    bool frameInFlight = false;
    bool stopAfterAck = false;
    bool singleStepRequested = false;
    std::uint64_t inFlightFrame = 0;
};
