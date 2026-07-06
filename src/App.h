#pragma once

class App
{
public:
    bool Init();
    void Run();
    void Shutdown();

private:
    bool running = true;
    void HandleEvents();
    void Update();
    void Render();
};