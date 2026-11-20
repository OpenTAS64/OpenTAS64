#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <string>
#include <vector>
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
    void RequestLoadProject();
    void RequestNewProject();
    void ContinuePendingProjectAction();
    void ConnectEmulatorPlugin();
    void RequestRestart(std::size_t replayFrames);
    void MarkProjectDirty();
    void Undo();
    void Redo();
    void ResetHistory();
    void LoadSettings();
    void SaveSettings();

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
    bool resetPending = false;
    bool replayAfterReset = false;
    std::size_t replayTarget = 0;
    std::uint64_t inFlightFrame = 0;
    bool projectDirty = false;
    bool autosaveEnabled = true;
    bool visualStickEnabled = true;
    Uint32 lastProjectEditTicks = 0;
    std::vector<TasProject> undoHistory;
    std::vector<TasProject> redoHistory;
    TasProject historyState;
    bool historyInitialized = false;
    int pendingProjectAction = 0; // 1 = load, 2 = new project
    bool unsavedPopupRequested = false;
    std::string lastSaveTime = "Not saved during this session";
};
