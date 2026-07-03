#include "App.h"

int main()
{
    App app;

    app.Init();

    while (true)
    {
    app.Render();
    }

    app.Shutdown();

    return 0;
}