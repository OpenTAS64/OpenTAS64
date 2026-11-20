#include "App.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

namespace {
std::string DescribeController(const ControllerState& c)
{
    std::string buttons;
    const auto add = [&buttons](bool pressed, const char* name) {
        if (!pressed) return;
        if (!buttons.empty()) buttons += " ";
        buttons += name;
    };

    add(c.a, "A"); add(c.b, "B"); add(c.z, "Z"); add(c.start, "Start");
    add(c.l, "L"); add(c.r, "R");
    add(c.dUp, "D-Up"); add(c.dDown, "D-Down");
    add(c.dLeft, "D-Left"); add(c.dRight, "D-Right");
    add(c.cUp, "C-Up"); add(c.cDown, "C-Down");
    add(c.cLeft, "C-Left"); add(c.cRight, "C-Right");

    if (buttons.empty()) buttons = "No buttons";
    buttons += "  |  Stick (" + std::to_string(static_cast<int>(c.stickX)) +
        ", " + std::to_string(static_cast<int>(c.stickY)) + ")";
    return buttons;
}
} // namespace

bool App::Init()
{
    project.EnsureFrame(119);
    {
        std::ifstream recentFile("last_project.txt");
        std::string recentPath;
        if (recentFile && std::getline(recentFile, recentPath) && !recentPath.empty() &&
            recentPath.size() < sizeof(projectPath)) {
            std::strncpy(projectPath, recentPath.c_str(), sizeof(projectPath) - 1);
            projectPath[sizeof(projectPath) - 1] = '\0';
        }
    }
    {
        std::string loadError;
        if (project.LoadFromFile(projectPath, &loadError)) {
            fileStatus = "Automatically restored " + std::string(projectPath);
            fileStatusOk = true;
        } else {
            fileStatus = "No previous project restored (" + loadError + ").";
            fileStatusOk = true;
        }
        projectDirty = false;
        ResetHistory();
    }
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("OpenTAS64", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    if (!window) { std::cerr << SDL_GetError() << std::endl; SDL_Quit(); return false; }
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window); window = nullptr; SDL_Quit(); return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window, renderer) ||
        !ImGui_ImplSDLRenderer2_Init(renderer)) {
        std::cerr << "Failed to initialize Dear ImGui SDL backends." << std::endl;
        Shutdown(); return false;
    }
    return true;
}
void App::Run() {
    while (running) { HandleEvents(); Update(); Render(); }
}
void App::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) running = false;
    }
}
void App::Update() {
    if (projectDirty && !frameInFlight &&
        static_cast<Uint32>(SDL_GetTicks() - lastProjectEditTicks) >= 750) {
        SaveProject();
    }
    if (resetPending) {
        bool success = false;
        if (connection.PollResetAcknowledgement(success)) {
            resetPending = false;
            if (!success) {
                replayAfterReset = false;
                connectionStatus = "Mupen64Plus rejected the reset command.";
            } else {
                project.SelectFrame(0);
                if (replayAfterReset && replayTarget > 0) {
                    playback = true;
                    connectionStatus = "Restarted emulator; replaying inputs to restore the requested TAS position.";
                } else {
                    playback = false;
                    connectionStatus = "Emulator restarted. TAS input data was preserved.";
                }
                replayAfterReset = false;
            }
        }
        return;
    }
    if (!connection.IsConnected()) {
        if (singleStepRequested) {
            singleStepRequested = false;
            connectionStatus = "Connection lost before the single frame step completed.";
        }
        if (frameInFlight) {
            playback = false;
            frameInFlight = false;
            stopAfterAck = false;
            connectionStatus = "Connection lost while waiting for frame acknowledgement.";
        }
        return;
    }
    if (!playback && !frameInFlight && !singleStepRequested) return;
    if (frameInFlight) {
        std::uint64_t acknowledged = 0;
        if (connection.PollAcknowledgement(acknowledged)) {
            if (acknowledged != inFlightFrame) {
                playback = false;
                frameInFlight = false;
                stopAfterAck = false;
                connectionStatus = "Protocol error: received an unexpected frame acknowledgement.";
                return;
            }
            frameInFlight = false;
            if (project.GetSelectedFrame() + 1 < project.GetFrameCount())
                project.SelectFrame(project.GetSelectedFrame() + 1);

            if (replayTarget > 0 && project.GetSelectedFrame() >= replayTarget) {
                playback = false;
                replayTarget = 0;
                connectionStatus = "Rewind complete. Emulator replayed from reset to the selected TAS position.";
            } else if (singleStepRequested) {
                singleStepRequested = false;
                connectionStatus = "One emulated frame completed. Ready for the next step.";
            } else if (stopAfterAck) {
                stopAfterAck = false;
                playback = false;
                connectionStatus = "Playback stopped safely after the in-flight frame completed.";
            } else if (project.GetSelectedFrame() + 1 >= project.GetFrameCount()) {
                playback = false;
                connectionStatus = "Playback finished. Last emulated frame reached the core frame boundary.";
            }
        }
        return;
    }
    if (!playback && !singleStepRequested) return;

    const auto frame = project.GetSelectedFrame();
    const auto& state = project.GetFrame(frame).controller;
    if (connection.SendFrame(0, static_cast<std::uint64_t>(frame), state)) {
        inFlightFrame = static_cast<std::uint64_t>(frame);
        frameInFlight = true;
        connectionStatus = "Sent frame " + std::to_string(frame) + "; waiting for the emulated frame to complete.";
    } else if (!connection.IsConnected()) {
        playback = false;
        connectionStatus = "Connection lost while sending frame.";
    }
}
void App::RequestRestart(std::size_t replayFrames) {
    if (!connection.IsConnected()) {
        connectionStatus = "Connect to the Mupen64Plus plugin first.";
        return;
    }
    if (frameInFlight || resetPending || playback) {
        connectionStatus = "Pause playback and wait for the current emulated frame to complete before restarting.";
        return;
    }
    if (connection.SendReset()) {
        resetPending = true;
        replayAfterReset = replayFrames > 0;
        replayTarget = replayFrames;
        playback = false;
        singleStepRequested = false;
        connectionStatus = "Reset command sent; waiting for Mupen64Plus.";
    }
}
void App::ConnectEmulatorPlugin() {
    if (connection.Connect()) connectionStatus = "Connected (protocol v3, 127.0.0.1:6464).";
    else connectionStatus = "Could not connect. Is the OpenTAS64 Mupen plugin loaded?";
}
void App::ResetHistory() {
    undoHistory.clear();
    redoHistory.clear();
    historyState = project;
    historyInitialized = true;
}
void App::MarkProjectDirty() {
    if (historyInitialized) {
        undoHistory.push_back(historyState);
        if (undoHistory.size() > 100) undoHistory.erase(undoHistory.begin());
        redoHistory.clear();
        historyState = project;
    }
    projectDirty = true;
    lastProjectEditTicks = SDL_GetTicks();
}
void App::Undo() {
    if (undoHistory.empty() || playback || frameInFlight || resetPending) return;
    redoHistory.push_back(project);
    project = undoHistory.back();
    undoHistory.pop_back();
    historyState = project;
    projectDirty = true;
    lastProjectEditTicks = SDL_GetTicks();
    fileStatus = "Undo applied.";
    fileStatusOk = true;
}
void App::Redo() {
    if (redoHistory.empty() || playback || frameInFlight || resetPending) return;
    undoHistory.push_back(project);
    project = redoHistory.back();
    redoHistory.pop_back();
    historyState = project;
    projectDirty = true;
    lastProjectEditTicks = SDL_GetTicks();
    fileStatus = "Redo applied.";
    fileStatusOk = true;
}
void App::SaveProject() {
    std::string error;
    if (project.SaveToFile(projectPath, &error)) {
        std::ofstream recentFile("last_project.txt", std::ios::trunc);
        if (recentFile) recentFile << projectPath << '\n';
        projectDirty = false;
        fileStatus = "Saved " + std::string(projectPath) + " (automatic save enabled)";
        fileStatusOk = true;
    } else { fileStatus = error; fileStatusOk = false; }
}
void App::LoadProject() {
    if (frameInFlight) {
        fileStatus = "Wait for the current emulator input frame to be acknowledged before loading.";
        fileStatusOk = false;
        return;
    }
    std::string error;
    if (project.LoadFromFile(projectPath, &error)) {
        projectDirty = false;
        ResetHistory();
        std::ofstream recentFile("last_project.txt", std::ios::trunc);
        if (recentFile) recentFile << projectPath << '\n';
        fileStatus = "Loaded " + std::string(projectPath); fileStatusOk = true;
    } else { fileStatus = error; fileStatusOk = false; }
}
void App::CreateNewProject() {
    if (frameInFlight) {
        fileStatus = "Wait for the current emulator input frame to be acknowledged before creating a project.";
        fileStatusOk = false;
        return;
    }
    playback = false; stopAfterAck = false;
    project.Clear(); project.EnsureFrame(119); project.SetName("Untitled TAS");
    ResetHistory();
    MarkProjectDirty();
    std::strncpy(projectPath, "project.otas", sizeof(projectPath) - 1);
    projectPath[sizeof(projectPath) - 1] = '\0';
    fileStatus = "New project created. Save it to keep your changes."; fileStatusOk = true;
}
void App::RenderInputEditor() {
    auto& c = project.GetFrame(project.GetSelectedFrame()).controller;
    ImGui::Text("Editing frame %zu", project.GetSelectedFrame());
    const bool editingLocked = playback || frameInFlight;
    if (editingLocked)
        ImGui::TextDisabled("Input editing is locked during playback or while a frame is in flight.");
    ImGui::BeginDisabled(editingLocked);
    ImGui::Separator();
    ImGui::Columns(3, nullptr, false);
    if (ImGui::Checkbox("A", &c.a)) MarkProjectDirty(); if (ImGui::Checkbox("B", &c.b)) MarkProjectDirty();
    if (ImGui::Checkbox("Z", &c.z)) MarkProjectDirty(); if (ImGui::Checkbox("Start", &c.start)) MarkProjectDirty();
    ImGui::NextColumn();
    if (ImGui::Checkbox("L", &c.l)) MarkProjectDirty(); if (ImGui::Checkbox("R", &c.r)) MarkProjectDirty();
    if (ImGui::Checkbox("D-Up", &c.dUp)) MarkProjectDirty(); if (ImGui::Checkbox("D-Down", &c.dDown)) MarkProjectDirty();
    ImGui::NextColumn();
    if (ImGui::Checkbox("D-Left", &c.dLeft)) MarkProjectDirty(); if (ImGui::Checkbox("D-Right", &c.dRight)) MarkProjectDirty();
    if (ImGui::Checkbox("C-Up", &c.cUp)) MarkProjectDirty(); if (ImGui::Checkbox("C-Down", &c.cDown)) MarkProjectDirty();
    if (ImGui::Checkbox("C-Left", &c.cLeft)) MarkProjectDirty(); if (ImGui::Checkbox("C-Right", &c.cRight)) MarkProjectDirty();
    ImGui::Columns(1); ImGui::Separator();
    int x = c.stickX, y = c.stickY;
    if (ImGui::SliderInt("Stick X", &x, -128, 127)) { c.stickX = static_cast<std::int8_t>(x); MarkProjectDirty(); }
    if (ImGui::SliderInt("Stick Y", &y, -128, 127)) { c.stickY = static_cast<std::int8_t>(y); MarkProjectDirty(); }
    ImGui::Text("Raw stick: (%d, %d)", static_cast<int>(c.stickX), static_cast<int>(c.stickY));
    ImGui::EndDisabled();
}
void App::RenderTimeline() {
    const auto count = project.GetFrameCount();
    const auto selected = project.GetSelectedFrame();
    ImGui::Text("Frames: %zu  |  Selected frame: %zu", count, selected);
    ImGui::SameLine();
    ImGui::BeginDisabled(playback || frameInFlight);
    if (ImGui::Button("Previous") && selected > 0) {
        project.SelectFrame(selected - 1);
        MarkProjectDirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("Next") && selected + 1 < count) {
        project.SelectFrame(selected + 1);
        MarkProjectDirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("Insert")) { project.InsertFrame(selected); MarkProjectDirty(); }
    ImGui::SameLine();
    if (ImGui::Button("Delete") && project.DeleteFrame(selected)) MarkProjectDirty();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!connection.IsConnected() || playback || frameInFlight || resetPending || singleStepRequested);
    if (ImGui::Button("Rewind to selected")) {
        if (selected > 0) RequestRestart(selected);
        else RequestRestart(0);
    }
    ImGui::SameLine();
    if (ImGui::Button("Restart Emulator")) RequestRestart(0);
    ImGui::EndDisabled();
    if (!playback && !frameInFlight && !singleStepRequested && !resetPending) {
        if (ImGui::Button("Step Frame")) {
            if (connection.IsConnected()) {
                singleStepRequested = true;
                connectionStatus = "Sending one frame; waiting for its input-poll acknowledgement.";
            } else connectionStatus = "Connect to the Mupen64Plus plugin first.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Play from selected frame")) {
            if (connection.IsConnected()) {
                playback = true;
                stopAfterAck = false;
                connectionStatus = "Lockstep playback started.";
            } else connectionStatus = "Connect to the Mupen64Plus plugin first.";
        }
    } else if (stopAfterAck) {
        ImGui::BeginDisabled();
        ImGui::Button("Pausing...");
        ImGui::EndDisabled();
    } else if (ImGui::Button("Pause")) {
        if (frameInFlight) {
            stopAfterAck = true;
            playback = false;
            connectionStatus = "Pause requested; the current input poll will finish before the emulator pauses.";
        } else {
            playback = false;
            connectionStatus = "Playback paused.";
        }
    }
    ImGui::Separator();
    ImGui::TextWrapped("%s", connectionStatus.c_str());
    ImGui::TextDisabled("Select a frame to edit its inputs in the Input Editor. Button states and stick coordinates are previewed below.");
    static constexpr std::array<std::pair<const char*, bool ControllerState::*>, 14> buttonColumns{{
        {"A", &ControllerState::a}, {"B", &ControllerState::b},
        {"Z", &ControllerState::z}, {"Start", &ControllerState::start},
        {"L", &ControllerState::l}, {"R", &ControllerState::r},
        {"D-Up", &ControllerState::dUp}, {"D-Down", &ControllerState::dDown},
        {"D-Left", &ControllerState::dLeft}, {"D-Right", &ControllerState::dRight},
        {"C-Up", &ControllerState::cUp}, {"C-Down", &ControllerState::cDown},
        {"C-Left", &ControllerState::cLeft}, {"C-Right", &ControllerState::cRight}
    }};

    ImGui::TextDisabled("Piano Roll: all frames are listed. Select a row to edit it; click a cell to toggle an input.");
    ImGui::BeginChild("PianoRoll", ImVec2(0, 0), true);
    if (ImGui::BeginTable("PianoRollTable", 17,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 76.0f);
        for (const auto& column : buttonColumns)
            ImGui::TableSetupColumn(column.first, ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("Stick X", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("Stick Y", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableHeadersRow();
        for (std::size_t i = 0; i < count; ++i) {
            auto& controller = project.GetFrame(i).controller;
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginDisabled(playback || frameInFlight);
            const auto frameLabel = std::string(i == selected ? "> " : "") + std::to_string(i + 1);
            if (ImGui::Selectable(frameLabel.c_str(), i == selected, ImGuiSelectableFlags_SpanAllColumns)) {
                project.SelectFrame(i);
                MarkProjectDirty();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(playback || frameInFlight);
            for (std::size_t column = 0; column < buttonColumns.size(); ++column) {
                ImGui::TableSetColumnIndex(static_cast<int>(column + 1));
                ImGui::PushID(static_cast<int>(column));
                if (ImGui::Checkbox("##pressed", &(controller.*(buttonColumns[column].second)))) MarkProjectDirty();
                ImGui::PopID();
            }
            ImGui::TableSetColumnIndex(15);
            int stickX = controller.stickX;
            if (ImGui::SetNextItemWidth(58.0f), ImGui::InputInt("##stickx", &stickX, 0, 0)) {
                controller.stickX = static_cast<std::int8_t>(std::clamp(stickX, -128, 127));
                MarkProjectDirty();
            }
            ImGui::TableSetColumnIndex(16);
            int stickY = controller.stickY;
            if (ImGui::SetNextItemWidth(58.0f), ImGui::InputInt("##sticky", &stickY, 0, 0)) {
                controller.stickY = static_cast<std::int8_t>(std::clamp(stickY, -128, 127));
                MarkProjectDirty();
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}
void App::Render() {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_NewFrame(); ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    const auto flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;
    ImGui::Begin("OpenTAS64 Dockspace", nullptr, flags);
    ImGui::DockSpace(ImGui::GetID("MainDockSpace")); ImGui::End();
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New project")) CreateNewProject();
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !undoHistory.empty() && !playback && !frameInFlight)) Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !redoHistory.empty() && !playback && !frameInFlight)) Redo();
            ImGui::Separator();
            if (ImGui::MenuItem("Save project", "Ctrl+S")) SaveProject();
            if (ImGui::MenuItem("Load project")) {
                if (!frameInFlight) { playback = false; LoadProject(); }
                else { fileStatus = "Wait for the in-flight frame acknowledgement before loading."; fileStatusOk = false; }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) running = false;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S)) SaveProject();
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
    ImGui::Begin("Project");
    ImGui::Text("Name: %s", project.GetName().c_str());
    ImGui::InputText("File path (.otas)", projectPath, sizeof(projectPath));
    if (ImGui::Button("Save")) SaveProject();
    ImGui::SameLine(); if (ImGui::Button("Load")) {
        if (!frameInFlight) { playback = false; LoadProject(); }
        else { fileStatus = "Wait for the in-flight frame acknowledgement before loading."; fileStatusOk = false; }
    }
    ImGui::SameLine(); if (ImGui::Button("New")) CreateNewProject();
    ImGui::TextColored(fileStatusOk ? ImVec4(0.35f,0.85f,0.45f,1.0f) : ImVec4(1.0f,0.35f,0.3f,1.0f), "%s", fileStatus.c_str());
    ImGui::End();
    ImGui::Begin("Connection");
    ImGui::Text("Mupen64Plus plugin: %s", connection.IsConnected() ? "Connected" : "Not connected");
    ImGui::TextWrapped("Load the OpenTAS64 Network Input plugin in Mupen64Plus.");
    if (!connection.IsConnected()) {
        if (ImGui::Button("Connect to plugin")) ConnectEmulatorPlugin();
    } else if (ImGui::Button("Disconnect")) {
        playback = false; frameInFlight = false; connection.Disconnect();
        connectionStatus = "Disconnected.";
    }
    ImGui::Text("Target: 127.0.0.1:6464");
    ImGui::TextWrapped("Protocol v3: lockstep input frames plus a direct core reset command. Rewind rebuilds the state by replaying inputs from the beginning.");
    ImGui::End();
    ImGui::Begin("Input Editor"); RenderInputEditor(); ImGui::End();
    ImGui::Begin("Timeline"); RenderTimeline(); ImGui::End();
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}
void App::Shutdown() {
    if (projectDirty) SaveProject();
    connection.Disconnect();
    ImGui_ImplSDLRenderer2_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    renderer = nullptr; window = nullptr; SDL_Quit();
}
