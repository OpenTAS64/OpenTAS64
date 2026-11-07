#include "App.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

bool App::Init() {
    if(SDL_Init(SDL_INIT_VIDEO)!=0){std::cerr<<"SDL_Init Error: "<<SDL_GetError()<<std::endl;return false;}
    window=SDL_CreateWindow("OpenTAS64",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_SHOWN);
    if(!window){std::cerr<<"SDL_CreateWindow Error: "<<SDL_GetError()<<std::endl;SDL_Quit();return false;}
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!renderer){std::cerr<<"SDL_CreateRenderer Error: "<<SDL_GetError()<<std::endl;SDL_DestroyWindow(window);window=nullptr;SDL_Quit();return false;}
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    if(!ImGui_ImplSDL2_InitForSDLRenderer(window,renderer)||!ImGui_ImplSDLRenderer2_Init(renderer)){
        std::cerr<<"Failed to initialize Dear ImGui SDL backends."<<std::endl;Shutdown();return false;
    }
    return true;
}
void App::Run(){while(running){HandleEvents();Update();Render();}}
void App::HandleEvents(){SDL_Event event;while(SDL_PollEvent(&event)){ImGui_ImplSDL2_ProcessEvent(&event);if(event.type==SDL_QUIT)running=false;}}
void App::Update(){}

void App::RenderInputEditor(){
    auto& c=project.GetFrame(project.GetSelectedFrame()).controller;
    ImGui::Text("Editing frame %zu",project.GetSelectedFrame()); ImGui::Separator();
    ImGui::Columns(3,nullptr,false);
    ImGui::Checkbox("A",&c.a); ImGui::Checkbox("B",&c.b); ImGui::Checkbox("Z",&c.z); ImGui::Checkbox("Start",&c.start);
    ImGui::NextColumn();
    ImGui::Checkbox("L",&c.l); ImGui::Checkbox("R",&c.r); ImGui::Checkbox("D-Up",&c.dUp); ImGui::Checkbox("D-Down",&c.dDown);
    ImGui::NextColumn();
    ImGui::Checkbox("D-Left",&c.dLeft); ImGui::Checkbox("D-Right",&c.dRight);
    ImGui::Checkbox("C-Up",&c.cUp); ImGui::Checkbox("C-Down",&c.cDown);
    ImGui::Checkbox("C-Left",&c.cLeft); ImGui::Checkbox("C-Right",&c.cRight);
    ImGui::Columns(1); ImGui::Separator();
    int x=c.stickX,y=c.stickY;
    if(ImGui::SliderInt("Stick X",&x,-128,127))c.stickX=static_cast<std::int8_t>(x);
    if(ImGui::SliderInt("Stick Y",&y,-128,127))c.stickY=static_cast<std::int8_t>(y);
    ImGui::Text("Raw stick: (%d, %d)",static_cast<int>(c.stickX),static_cast<int>(c.stickY));
}
void App::RenderTimeline(){
    ImGui::Text("Frames: %zu",project.GetFrameCount());
    ImGui::SameLine(); if(ImGui::Button("Previous")&&project.GetSelectedFrame()>0)project.SelectFrame(project.GetSelectedFrame()-1);
    ImGui::SameLine(); if(ImGui::Button("Next"))project.SelectFrame(project.GetSelectedFrame()+1);
    ImGui::SameLine(); if(ImGui::Button("Insert"))project.InsertFrame(project.GetSelectedFrame());
    ImGui::SameLine(); if(ImGui::Button("Delete"))project.DeleteFrame(project.GetSelectedFrame());
    ImGui::Separator();
    const auto count=project.GetFrameCount(), selected=project.GetSelectedFrame();
    const auto start=selected>12?selected-12:0, end=std::min(count,start+40);
    ImGui::BeginChild("FrameStrip",ImVec2(0,0),true);
    for(std::size_t i=start;i<end;++i){
        if(i!=start)ImGui::SameLine();
        auto label=std::to_string(i); ImGui::PushID(static_cast<int>(i));
        if(ImGui::Selectable(label.c_str(),i==selected,0,ImVec2(42,38)))project.SelectFrame(i);
        ImGui::PopID();
    }
    ImGui::EndChild();
}
void App::Render(){
    SDL_SetRenderDrawColor(renderer,30,30,30,255); SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_NewFrame(); ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
    auto* viewport=ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(viewport->WorkSize); ImGui::SetNextWindowViewport(viewport->ID);
    const auto flags=ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoNavFocus;
    ImGui::Begin("OpenTAS64 Dockspace",nullptr,flags); ImGui::DockSpace(ImGui::GetID("MainDockSpace")); ImGui::End();
    if(ImGui::BeginMainMenuBar()){if(ImGui::BeginMenu("File")){if(ImGui::MenuItem("New project"))project.Clear();ImGui::EndMenu();}ImGui::EndMainMenuBar();}
    ImGui::Begin("Connection"); ImGui::Text("Emulator: Not connected");
    ImGui::TextWrapped("Emulator integration is not implemented yet. The TAS editor works offline.");
    ImGui::BeginDisabled(); ImGui::Button("Connect"); ImGui::EndDisabled(); ImGui::End();
    ImGui::Begin("Input Editor"); RenderInputEditor(); ImGui::End();
    ImGui::Begin("Timeline"); RenderTimeline(); ImGui::End();
    ImGui::Render(); ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),renderer); SDL_RenderPresent(renderer);
}
void App::Shutdown(){
    ImGui_ImplSDLRenderer2_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext();
    if(renderer)SDL_DestroyRenderer(renderer); if(window)SDL_DestroyWindow(window);
    renderer=nullptr;window=nullptr;SDL_Quit();
}
