#include "tas/TasProject.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
}

int main()
{
    const std::string path = "opentas64_test_project.otas";
    TasProject source("Round-trip \"Test\"");

    source.EnsureFrame(2);
    source.GetFrame(0).controller.a = true;
    source.GetFrame(1).controller.stickX = -128;
    source.GetFrame(1).controller.stickY = 127;
    source.GetFrame(2).controller.cRight = true;
    source.SelectFrame(2);

    std::string error;
    Check(source.SaveToFile(path, &error), "save a valid project");

    TasProject loaded("Should be replaced");
    Check(loaded.LoadFromFile(path, &error), "load a saved project");
    Check(loaded.GetName() == source.GetName(), "preserve project name");
    Check(loaded.GetFrameCount() == 3, "preserve frame count");
    Check(loaded.GetSelectedFrame() == 2, "preserve selected frame");
    Check(loaded.GetFrame(0).controller.a, "preserve A button");
    Check(loaded.GetFrame(1).controller.stickX == -128, "preserve minimum stick X");
    Check(loaded.GetFrame(1).controller.stickY == 127, "preserve maximum stick Y");
    Check(loaded.GetFrame(2).controller.cRight, "preserve C-right button");

    const auto previousName = loaded.GetName();
    {
        std::FILE* bad = std::fopen(path.c_str(), "w");
        if (bad)
        {
            std::fputs("NOT_A_PROJECT\n", bad);
            std::fclose(bad);
        }
    }
    Check(!loaded.LoadFromFile(path, &error), "reject an invalid project");
    Check(loaded.GetName() == previousName, "leave current project untouched after failed load");

    loaded.InsertFrame(1);
    Check(loaded.GetFrameCount() == 4, "insert a frame");
    Check(loaded.DeleteFrame(1), "delete a frame");
    Check(loaded.GetFrameCount() == 3, "frame count after deletion");
    Check(!loaded.DeleteFrame(99), "reject deletion outside the timeline");
    Check(loaded.DeleteFrame(0), "delete first frame");
    Check(loaded.DeleteFrame(0), "delete second frame");
    Check(!loaded.DeleteFrame(0), "keep at least one frame");

    std::remove(path.c_str());
    if (failures == 0)
    {
        std::cout << "All TasProject tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
}
