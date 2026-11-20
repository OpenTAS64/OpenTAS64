#include "App.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

namespace {
constexpr std::size_t kMaximumTimelineFrames = 1000000;

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
    LoadSettings();
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
            currentProjectPath = projectPath;
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
    if (autosaveEnabled && projectDirty && !frameInFlight &&
        static_cast<Uint32>(SDL_GetTicks() - lastProjectEditTicks) >= 750) {
        SaveProject();
    }
    if (resetPending) {
        bool success = false;
        if (connection.PollResetAcknowledgement(success)) {
            resetPending = false;
            if (!success) {
                replayAfterReset = false;
                replayTarget = 0;
                connectionStatus = "The plugin received the reset request, but Mupen64Plus rejected it. Check that the emulator core is running and initialized.";
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
        } else if (!connection.IsConnected()) {
            resetPending = false;
            replayAfterReset = false;
            replayTarget = 0;
            playback = false;
            connectionStatus = "Connection lost while waiting for the emulator reset acknowledgement. Reconnect before trying again.";
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
    } else if (!connection.IsConnected()) {
        connectionStatus = "Connection lost while sending the reset command. Reconnect to the plugin and try again.";
    } else {
        connectionStatus = "The reset command could not be sent. Make sure no frame or reset request is still pending, then try again.";
    }
}
void App::ConnectEmulatorPlugin() {
    if (connection.Connect()) connectionStatus = "Connected (protocol v3, 127.0.0.1:6464).";
    else connectionStatus = "Could not connect to 127.0.0.1:6464. Start Mupen64Plus with the OpenTAS64 Network Input plugin loaded, then retry.";
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
void App::LoadSettings() {
    std::ifstream settingsFile("opentas64_settings.ini");
    std::string line;
    while (std::getline(settingsFile, line)) {
        if (line == "autosave=0") autosaveEnabled = false;
        else if (line == "autosave=1") autosaveEnabled = true;
        else if (line == "stick_editor=sliders") visualStickEnabled = false;
        else if (line == "stick_editor=visual") visualStickEnabled = true;
    }
}
void App::SaveSettings() {
    std::ofstream settingsFile("opentas64_settings.ini", std::ios::trunc);
    if (settingsFile) {
        settingsFile << "autosave=" << (autosaveEnabled ? "1" : "0") << '\n';
        settingsFile << "stick_editor=" << (visualStickEnabled ? "visual" : "sliders") << '\n';
    }
}
void App::SaveProject() {
    std::string error;
    if (project.SaveToFile(projectPath, &error)) {
        std::ofstream recentFile("last_project.txt", std::ios::trunc);
        if (recentFile) recentFile << projectPath << '\n';
        projectDirty = false;
        currentProjectPath = projectPath;
        const std::time_t savedAt = std::time(nullptr);
        const std::tm* localTime = std::localtime(&savedAt);
        if (localTime) {
            char timeBuffer[64]{};
            if (std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime) > 0)
                lastSaveTime = timeBuffer;
        }
        fileStatus = "Saved " + std::string(projectPath) + (autosaveEnabled ? " (automatic saving enabled)" : " (automatic saving disabled)");
        fileStatusOk = true;
    } else { fileStatus = error; fileStatusOk = false; }
}
void App::SaveActiveProject() {
    const std::string requestedPath = projectPath;
    if (!currentProjectPath.empty()) {
        std::strncpy(projectPath, currentProjectPath.c_str(), sizeof(projectPath) - 1);
        projectPath[sizeof(projectPath) - 1] = '\0';
    }
    SaveProject();
    std::strncpy(projectPath, requestedPath.c_str(), sizeof(projectPath) - 1);
    projectPath[sizeof(projectPath) - 1] = '\0';
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
        currentProjectPath = projectPath;
        ResetHistory();
        std::ofstream recentFile("last_project.txt", std::ios::trunc);
        if (recentFile) recentFile << projectPath << '\n';
        fileStatus = "Loaded " + std::string(projectPath); fileStatusOk = true;
    } else { fileStatus = error; fileStatusOk = false; }
}
void App::RequestLoadProject() {
    if (frameInFlight || resetPending) {
        fileStatus = "Wait for the current emulator operation to finish before loading a project.";
        fileStatusOk = false;
        return;
    }
    if (projectDirty && !autosaveEnabled) {
        pendingProjectAction = 1;
        unsavedPopupRequested = true;
        return;
    }
    if (projectDirty && autosaveEnabled) {
        SaveActiveProject();
        if (projectDirty) return;
    }
    playback = false;
    LoadProject();
}
void App::RequestNewProject() {
    if (frameInFlight || resetPending) {
        fileStatus = "Wait for the current emulator operation to finish before creating a project.";
        fileStatusOk = false;
        return;
    }
    if (projectDirty && !autosaveEnabled) {
        pendingProjectAction = 2;
        unsavedPopupRequested = true;
        return;
    }
    if (projectDirty && autosaveEnabled) {
        SaveActiveProject();
        if (projectDirty) return;
    }
    CreateNewProject();
}
void App::ContinuePendingProjectAction() {
    const int action = pendingProjectAction;
    pendingProjectAction = 0;
    if (action == 1) {
        playback = false;
        LoadProject();
    } else if (action == 2) {
        CreateNewProject();
    }
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
    currentProjectPath = projectPath;
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
    ImGui::Text("Analog Stick");
    if (visualStickEnabled) {
    const float stickSize = 184.0f;
    const float stickRadius = 76.0f;
    const ImVec2 padTopLeft = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("StickPad", ImVec2(stickSize, stickSize));
    const bool stickPadActive = ImGui::IsItemActive();
    const ImVec2 padCenter(padTopLeft.x + stickSize * 0.5f,
                           padTopLeft.y + stickSize * 0.5f);
    if (stickPadActive && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        float dx = (mouse.x - padCenter.x) / stickRadius;
        float dy = (padCenter.y - mouse.y) / stickRadius;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length > 1.0f) { dx /= length; dy /= length; }
        const int nextX = std::clamp(static_cast<int>(std::lround(dx * 127.0f)), -128, 127);
        const int nextY = std::clamp(static_cast<int>(std::lround(dy * 127.0f)), -128, 127);
        if (c.stickX != nextX || c.stickY != nextY) {
            c.stickX = static_cast<std::int8_t>(nextX);
            c.stickY = static_cast<std::int8_t>(nextY);
            MarkProjectDirty();
        }
    }
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 outerColor = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 axisColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImU32 knobColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
    draw->AddCircle(padCenter, stickRadius, outerColor, 64, 2.0f);
    draw->AddCircle(padCenter, stickRadius * 0.5f, axisColor, 48, 1.0f);
    draw->AddLine(ImVec2(padCenter.x - stickRadius, padCenter.y),
                  ImVec2(padCenter.x + stickRadius, padCenter.y), axisColor, 1.0f);
    draw->AddLine(ImVec2(padCenter.x, padCenter.y - stickRadius),
                  ImVec2(padCenter.x, padCenter.y + stickRadius), axisColor, 1.0f);
    const float handleX = padCenter.x + (static_cast<float>(c.stickX) / 127.0f) * stickRadius;
    const float handleY = padCenter.y - (static_cast<float>(c.stickY) / 127.0f) * stickRadius;
    draw->AddCircleFilled(ImVec2(handleX, handleY), 10.0f, knobColor, 32);
    draw->AddCircle(ImVec2(handleX, handleY), 10.0f, outerColor, 32, 1.5f);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("X: %d", static_cast<int>(c.stickX));
    ImGui::Text("Y: %d", static_cast<int>(c.stickY));
    ImGui::TextDisabled("Drag the stick\nto set a direction.");
    if (ImGui::Button("Center stick")) {
        if (c.stickX != 0 || c.stickY != 0) {
            c.stickX = 0;
            c.stickY = 0;
            MarkProjectDirty();
        }
    }
    ImGui::EndGroup();
    ImGui::Text("Raw stick: (%d, %d)", static_cast<int>(c.stickX), static_cast<int>(c.stickY));
    } else {
        int stickX = c.stickX;
        int stickY = c.stickY;
        if (ImGui::SliderInt("Stick X", &stickX, -128, 127)) {
            c.stickX = static_cast<std::int8_t>(stickX);
            MarkProjectDirty();
        }
        if (ImGui::SliderInt("Stick Y", &stickY, -128, 127)) {
            c.stickY = static_cast<std::int8_t>(stickY);
            MarkProjectDirty();
        }
        if (ImGui::Button("Center stick")) {
            if (c.stickX != 0 || c.stickY != 0) {
                c.stickX = 0;
                c.stickY = 0;
                MarkProjectDirty();
            }
        }
        ImGui::Text("Raw stick: (%d, %d)", static_cast<int>(c.stickX), static_cast<int>(c.stickY));
    }
    ImGui::EndDisabled();
}
void App::RenderTimeline() {
    const auto count = project.GetFrameCount();
    const auto selected = project.GetSelectedFrame();
    ImGui::Text("Frames: %zu  |  Selected frame: %zu", count, selected);
    const std::size_t remainingFrames = count < kMaximumTimelineFrames
        ? kMaximumTimelineFrames - count : 0;
    ImGui::TextDisabled("Extend timeline:");
    ImGui::SameLine();
    ImGui::BeginDisabled(playback || frameInFlight || remainingFrames == 0);
    if (ImGui::Button("Add frame")) {
        project.InsertFrame(count);
        project.SelectFrame(count);
        MarkProjectDirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add 10 frames")) {
        const std::size_t framesToAdd = std::min<std::size_t>(10, remainingFrames);
        for (std::size_t i = 0; i < framesToAdd; ++i)
            project.InsertFrame(project.GetFrameCount());
        project.SelectFrame(count);
        MarkProjectDirty();
    }
    ImGui::EndDisabled();
    ImGui::Separator();
    ImGui::BeginDisabled(playback || frameInFlight);
    if (ImGui::Button("Previous") && selected > 0) {
        project.SelectFrame(selected - 1);
    }
    ImGui::SameLine();
    if (ImGui::Button("Next") && selected + 1 < count) {
        project.SelectFrame(selected + 1);
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

    ImGui::TextDisabled("Piano Roll: all frames are listed. Use Add frame / Add 10 frames above to extend the timeline; select a row to edit it.");
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
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(count));
        while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const std::size_t i = static_cast<std::size_t>(row);
            auto& controller = project.GetFrame(i).controller;
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginDisabled(playback || frameInFlight);
            const auto frameLabel = std::string(i == selected ? "> " : "") + std::to_string(i + 1);
            if (ImGui::Selectable(frameLabel.c_str(), i == selected, ImGuiSelectableFlags_SpanAllColumns)) {
                project.SelectFrame(i);
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
            if (ImGui::MenuItem("New project")) RequestNewProject();
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !undoHistory.empty() && !playback && !frameInFlight)) Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !redoHistory.empty() && !playback && !frameInFlight)) Redo();
            ImGui::Separator();
            if (ImGui::MenuItem("Save project", "Ctrl+S")) SaveProject();
            if (ImGui::MenuItem("Load project")) RequestLoadProject();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Automatic saving", nullptr, &autosaveEnabled)) SaveSettings();
            ImGui::Separator();
            if (ImGui::MenuItem("Visual stick pad", nullptr, visualStickEnabled, !visualStickEnabled)) {
                visualStickEnabled = true;
                SaveSettings();
            }
            if (ImGui::MenuItem("Stick X/Y sliders", nullptr, !visualStickEnabled, visualStickEnabled)) {
                visualStickEnabled = false;
                SaveSettings();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    const ImGuiIO& io = ImGui::GetIO();
    const bool shortcutsAllowed = !io.WantTextInput && !io.WantCaptureKeyboard &&
        !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);
    const bool controlDown = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    if (shortcutsAllowed) {
        if (controlDown && ImGui::IsKeyPressed(ImGuiKey_S)) SaveProject();
        if (controlDown && ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
        if (controlDown && ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
        if (!controlDown && ImGui::IsKeyPressed(ImGuiKey_Space)) {
            if (playback) {
                if (frameInFlight) {
                    stopAfterAck = true;
                    playback = false;
                    connectionStatus = "Pause requested; the current input poll will finish before playback stops.";
                } else {
                    playback = false;
                    connectionStatus = "Playback paused.";
                }
            } else if (connection.IsConnected() && !frameInFlight && !resetPending && !singleStepRequested) {
                stopAfterAck = false;
                playback = true;
                connectionStatus = "Lockstep playback started.";
            } else if (!connection.IsConnected()) {
                connectionStatus = "Connect to the Mupen64Plus plugin before starting playback.";
            }
        }
        if (!playback && !frameInFlight && !resetPending && !singleStepRequested) {
            if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
                project.SelectFrame(0);
                connectionStatus = "Selected the first TAS frame.";
            } else if (ImGui::IsKeyPressed(ImGuiKey_End)) {
                project.SelectFrame(project.GetFrameCount() - 1);
                connectionStatus = "Selected the last TAS frame.";
            }
        }
    }
    ImGui::Begin("Project");
    ImGui::Text("Name: %s", project.GetName().c_str());
    ImGui::InputText("File path (.otas)", projectPath, sizeof(projectPath));
    ImGui::TextColored(projectDirty ? ImVec4(1.0f, 0.68f, 0.22f, 1.0f) : ImVec4(0.35f, 0.85f, 0.45f, 1.0f),
        "%s", projectDirty ? "Unsaved changes" : "All changes saved");
    ImGui::SameLine();
    ImGui::TextDisabled("Autosave: %s | Last save: %s", autosaveEnabled ? "On" : "Off", lastSaveTime.c_str());
    if (ImGui::Button("Save")) SaveProject();
    ImGui::SameLine(); if (ImGui::Button("Load")) RequestLoadProject();
    ImGui::SameLine(); if (ImGui::Button("New")) RequestNewProject();
    ImGui::Separator();
    ImGui::TextDisabled("Edit history");
    const bool historyLocked = playback || frameInFlight || resetPending;
    ImGui::BeginDisabled(historyLocked || undoHistory.empty());
    if (ImGui::Button("Undo  (Ctrl+Z)", ImVec2(132.0f, 0.0f))) Undo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(historyLocked || redoHistory.empty());
    if (ImGui::Button("Redo  (Ctrl+Y)", ImVec2(132.0f, 0.0f))) Redo();
    ImGui::EndDisabled();
    ImGui::TextColored(fileStatusOk ? ImVec4(0.35f,0.85f,0.45f,1.0f) : ImVec4(1.0f,0.35f,0.3f,1.0f), "%s", fileStatus.c_str());
    if (unsavedPopupRequested) {
        ImGui::OpenPopup("Unsaved changes");
        unsavedPopupRequested = false;
    }
    if (ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("This project has unsaved changes and automatic saving is disabled.");
        ImGui::TextWrapped(pendingProjectAction == 1
            ? "Loading another project will discard these changes."
            : "Creating a new project will discard these changes.");
        if (ImGui::Button("Save and continue", ImVec2(150.0f, 0.0f))) {
            SaveActiveProject();
            if (!projectDirty) {
                ImGui::CloseCurrentPopup();
                ContinuePendingProjectAction();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard changes", ImVec2(150.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
            ContinuePendingProjectAction();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f))) {
            pendingProjectAction = 0;
            unsavedPopupRequested = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
    ImGui::Begin("Connection");
    ImGui::Text("Mupen64Plus plugin: %s", connection.IsConnected() ? "Connected" : "Not connected");
    ImGui::TextWrapped("Load the OpenTAS64 Network Input plugin in Mupen64Plus.");
    if (!connection.IsConnected()) {
        if (ImGui::Button("Connect to plugin")) ConnectEmulatorPlugin();
    } else if (ImGui::Button("Disconnect")) {
        playback = false; frameInFlight = false; resetPending = false; replayAfterReset = false; connection.Disconnect();
        connectionStatus = "Disconnected.";
    }
    ImGui::Text("Target: 127.0.0.1:6464");
    ImGui::TextDisabled("Shortcuts: Space = Play/Pause | Home = First frame | End = Last frame | Ctrl+S = Save");
    ImGui::TextWrapped("Protocol v3: lockstep input frames plus a direct core reset command. Rewind rebuilds the state by replaying inputs from the beginning.");
    ImGui::End();
    ImGui::Begin("Input Editor"); RenderInputEditor(); ImGui::End();
    ImGui::Begin("Timeline"); RenderTimeline(); ImGui::End();
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}
void App::Shutdown() {
    if (autosaveEnabled && projectDirty) SaveProject();
    connection.Disconnect();
    ImGui_ImplSDLRenderer2_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    renderer = nullptr; window = nullptr; SDL_Quit();
}
