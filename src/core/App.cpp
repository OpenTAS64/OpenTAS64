#include "App.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
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
    if (!connection.IsConnected()) {
        if (frameInFlight) {
            playback = false;
            frameInFlight = false;
            stopAfterAck = false;
            connectionStatus = "Connection lost while waiting for frame acknowledgement.";
        }
        return;
    }
    if (!playback && !frameInFlight) return;
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

            if (stopAfterAck) {
                stopAfterAck = false;
                playback = false;
                connectionStatus = "Playback stopped safely after the in-flight frame was acknowledged.";
            } else if (project.GetSelectedFrame() + 1 >= project.GetFrameCount()) {
                playback = false;
                connectionStatus = "Playback finished. Last input frame was acknowledged by Mupen64Plus.";
            }
        }
        return;
    }
    if (!playback) return;

    const auto frame = project.GetSelectedFrame();
    const auto& state = project.GetFrame(frame).controller;
    if (connection.SendFrame(0, static_cast<std::uint64_t>(frame), state)) {
        inFlightFrame = static_cast<std::uint64_t>(frame);
        frameInFlight = true;
        connectionStatus = "Sent frame " + std::to_string(frame) + "; waiting for emulator input poll.";
    } else if (!connection.IsConnected()) {
        playback = false;
        connectionStatus = "Connection lost while sending frame.";
    }
}
void App::ConnectEmulatorPlugin() {
    if (connection.Connect()) connectionStatus = "Connected (protocol v2, 127.0.0.1:6464).";
    else connectionStatus = "Could not connect. Is the OpenTAS64 Mupen plugin loaded?";
}
void App::SaveProject() {
    std::string error;
    if (project.SaveToFile(projectPath, &error)) {
        fileStatus = "Saved " + std::string(projectPath); fileStatusOk = true;
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
    project.Clear(); project.SetName("Untitled TAS");
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
    ImGui::Checkbox("A", &c.a); ImGui::Checkbox("B", &c.b);
    ImGui::Checkbox("Z", &c.z); ImGui::Checkbox("Start", &c.start);
    ImGui::NextColumn();
    ImGui::Checkbox("L", &c.l); ImGui::Checkbox("R", &c.r);
    ImGui::Checkbox("D-Up", &c.dUp); ImGui::Checkbox("D-Down", &c.dDown);
    ImGui::NextColumn();
    ImGui::Checkbox("D-Left", &c.dLeft); ImGui::Checkbox("D-Right", &c.dRight);
    ImGui::Checkbox("C-Up", &c.cUp); ImGui::Checkbox("C-Down", &c.cDown);
    ImGui::Checkbox("C-Left", &c.cLeft); ImGui::Checkbox("C-Right", &c.cRight);
    ImGui::Columns(1); ImGui::Separator();
    int x = c.stickX, y = c.stickY;
    if (ImGui::SliderInt("Stick X", &x, -128, 127)) c.stickX = static_cast<std::int8_t>(x);
    if (ImGui::SliderInt("Stick Y", &y, -128, 127)) c.stickY = static_cast<std::int8_t>(y);
    ImGui::Text("Raw stick: (%d, %d)", static_cast<int>(c.stickX), static_cast<int>(c.stickY));
    ImGui::EndDisabled();
}
void App::RenderTimeline() {
    const auto count = project.GetFrameCount();
    const auto selected = project.GetSelectedFrame();
    ImGui::Text("Frames: %zu  |  Selected frame: %zu", count, selected);
    ImGui::SameLine();
    ImGui::BeginDisabled(playback || frameInFlight);
    if (ImGui::Button("Previous") && selected > 0)
        project.SelectFrame(selected - 1);
    ImGui::SameLine();
    if (ImGui::Button("Next") && selected + 1 < count)
        project.SelectFrame(selected + 1);
    ImGui::SameLine();
    if (ImGui::Button("Insert")) project.InsertFrame(selected);
    ImGui::SameLine();
    if (ImGui::Button("Delete")) project.DeleteFrame(selected);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (!playback && !frameInFlight) {
        if (ImGui::Button("Play from selected frame")) {
            if (connection.IsConnected()) {
                playback = true;
                stopAfterAck = false;
                connectionStatus = "Lockstep playback started.";
            } else connectionStatus = "Connect to the Mupen64Plus plugin first.";
        }
    } else if (stopAfterAck) {
        ImGui::BeginDisabled();
        ImGui::Button("Stopping...");
        ImGui::EndDisabled();
    } else if (ImGui::Button("Stop")) {
        if (frameInFlight) {
            stopAfterAck = true;
            playback = false;
            connectionStatus = "Stop requested; waiting for the current input frame to be acknowledged.";
        } else {
            playback = false;
            connectionStatus = "Playback stopped.";
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
    const auto start = selected > 12 ? selected - 12 : 0;
    const auto end = std::min(count, start + 40);
    ImGui::TextDisabled("Piano Roll: columns are controller inputs; each row is one frame. Click a cell to toggle an input.");
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
        for (std::size_t i = start; i < end; ++i) {
            auto& controller = project.GetFrame(i).controller;
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginDisabled(playback || frameInFlight);
            const auto frameLabel = std::string(i == selected ? "> " : "") + std::to_string(i + 1);
            if (ImGui::Selectable(frameLabel.c_str(), i == selected, ImGuiSelectableFlags_SpanAllColumns))
                project.SelectFrame(i);
            ImGui::EndDisabled();
            ImGui::BeginDisabled(playback || frameInFlight);
            for (std::size_t column = 0; column < buttonColumns.size(); ++column) {
                ImGui::TableSetColumnIndex(static_cast<int>(column + 1));
                ImGui::PushID(static_cast<int>(column));
                ImGui::Checkbox("##pressed", &(controller.*(buttonColumns[column].second)));
                ImGui::PopID();
            }
            ImGui::TableSetColumnIndex(15);
            int stickX = controller.stickX;
            if (ImGui::SetNextItemWidth(58.0f), ImGui::InputInt("##stickx", &stickX, 0, 0))
                controller.stickX = static_cast<std::int8_t>(std::clamp(stickX, -128, 127));
            ImGui::TableSetColumnIndex(16);
            int stickY = controller.stickY;
            if (ImGui::SetNextItemWidth(58.0f), ImGui::InputInt("##sticky", &stickY, 0, 0))
                controller.stickY = static_cast<std::int8_t>(std::clamp(stickY, -128, 127));
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
    ImGui::TextWrapped("Protocol v2: one frame is sent and acknowledged at the emulator's GetKeys input poll before the next frame is sent.");
    ImGui::End();
    ImGui::Begin("Input Editor"); RenderInputEditor(); ImGui::End();
    ImGui::Begin("Timeline"); RenderTimeline(); ImGui::End();
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}
void App::Shutdown() {
    connection.Disconnect();
    ImGui_ImplSDLRenderer2_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    renderer = nullptr; window = nullptr; SDL_Quit();
}
