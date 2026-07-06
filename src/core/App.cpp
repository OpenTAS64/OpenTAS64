#include "App.h"

#include <SDL2/SDL.h>
#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

bool App::Init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
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
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        return false;
    }


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

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
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT)
        {
            running = false;
        }
    }
};

void App::Update()
{
    // Emulator
    // Input
    // UI
}

void App::Render()
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    float width = io.DisplaySize.x;
    float height = io.DisplaySize.y;

    // =========================
    // FULLSCREEN DOCKSPACE
    // =========================
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace", nullptr, window_flags);

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id);

    ImGui::End();

    // =========================
    // CONNECTION PANEL
    // =========================
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(280, height - 240), ImGuiCond_Always);

    ImGui::Begin("Connection");

    ImGui::Text("Status:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Disconnected");

    ImGui::Separator();

    if (ImGui::Button("Connect"))
    {
        // TODO: Connect to emulator
    }

    ImGui::End();

    // =========================
    // INPUT PANEL
    // =========================
    ImGui::SetNextWindowPos(ImVec2(width - 290, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(280, height - 240), ImGuiCond_Always);

    ImGui::Begin("Input");

    ImGui::Text("Controller");
    ImGui::Separator();

    ImGui::Text("A      : -");
    ImGui::Text("B      : -");
    ImGui::Text("Start  : -");
    ImGui::Text("Z      : -");
    ImGui::Text("L      : -");
    ImGui::Text("R      : -");

    ImGui::Separator();

    ImGui::Text("Analog Stick");
    ImGui::Text("X: 0");
    ImGui::Text("Y: 0");

    ImGui::End();

    // =========================
    // TIMELINE PANEL
    // =========================
    ImGui::SetNextWindowPos(ImVec2(10, height - 220), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width - 20, 210), ImGuiCond_Always);

    ImGui::Begin("Timeline");

    ImGui::Text("Current Frame: 0");
    ImGui::Separator();
    ImGui::Text("Timeline will be implemented in v0.2");

    ImGui::End();

    ImGui::Render();

    ImGui_ImplSDLRenderer2_RenderDrawData(
        ImGui::GetDrawData(),
        renderer
    );

    SDL_RenderPresent(renderer);
}

void App::Shutdown()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}