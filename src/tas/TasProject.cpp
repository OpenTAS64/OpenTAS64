#include "TasProject.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <utility>
#include <vector>

namespace {
constexpr std::size_t kMaximumFrameCount = 1000000;

void SetError(std::string* error, const std::string& message)
{
    if (error) *error = message;
}

bool ReadController(std::istream& in, InputFrame& frame)
{
    int values[14]{};
    int x = 0;
    int y = 0;

    for (int& value : values)
    {
        if (!(in >> value) || (value != 0 && value != 1))
            return false;
    }

    if (!(in >> x >> y) || x < -128 || x > 127 || y < -128 || y > 127)
        return false;

    auto& c = frame.controller;
    c.a = values[0] != 0; c.b = values[1] != 0;
    c.z = values[2] != 0; c.start = values[3] != 0;
    c.dUp = values[4] != 0; c.dDown = values[5] != 0;
    c.dLeft = values[6] != 0; c.dRight = values[7] != 0;
    c.l = values[8] != 0; c.r = values[9] != 0;
    c.cUp = values[10] != 0; c.cDown = values[11] != 0;
    c.cLeft = values[12] != 0; c.cRight = values[13] != 0;
    c.stickX = static_cast<std::int8_t>(x);
    c.stickY = static_cast<std::int8_t>(y);
    return true;
}

void WriteController(std::ostream& out, const ControllerState& c)
{
    out << c.a << ' ' << c.b << ' ' << c.z << ' ' << c.start << ' '
        << c.dUp << ' ' << c.dDown << ' ' << c.dLeft << ' ' << c.dRight << ' '
        << c.l << ' ' << c.r << ' ' << c.cUp << ' ' << c.cDown << ' '
        << c.cLeft << ' ' << c.cRight << ' '
        << static_cast<int>(c.stickX) << ' ' << static_cast<int>(c.stickY) << '\n';
}
}

TasProject::TasProject(std::string value) : name(std::move(value)), frames(1) {}
const std::string& TasProject::GetName() const { return name; }
void TasProject::SetName(const std::string& value) { name = value; }
std::size_t TasProject::GetFrameCount() const { return frames.size(); }
std::size_t TasProject::GetSelectedFrame() const { return selectedFrame; }

bool TasProject::SelectFrame(std::size_t frame)
{
    if (frame >= frames.size()) return false;
    selectedFrame = frame;
    return true;
}

void TasProject::EnsureFrame(std::size_t frame)
{
    if (frame >= frames.size()) frames.resize(frame + 1);
}

void TasProject::InsertFrame(std::size_t frame)
{
    frame = std::min(frame, frames.size());
    frames.insert(frames.begin() + static_cast<std::ptrdiff_t>(frame), InputFrame{});
    if (selectedFrame >= frame) ++selectedFrame;
    if (selectedFrame >= frames.size()) selectedFrame = frames.size() - 1;
}

bool TasProject::DeleteFrame(std::size_t frame)
{
    if (frames.size() <= 1 || frame >= frames.size()) return false;
    frames.erase(frames.begin() + static_cast<std::ptrdiff_t>(frame));
    if (selectedFrame > frame) --selectedFrame;
    else if (selectedFrame == frame && selectedFrame >= frames.size())
        selectedFrame = frames.size() - 1;
    return true;
}

InputFrame& TasProject::GetFrame(std::size_t frame)
{
    EnsureFrame(frame);
    return frames[frame];
}

const InputFrame& TasProject::GetFrame(std::size_t frame) const
{
    return frames.at(frame);
}

void TasProject::Clear()
{
    frames.assign(1, InputFrame{});
    selectedFrame = 0;
}

bool TasProject::SaveToFile(const std::string& path, std::string* error) const
{
    if (path.empty())
    {
        SetError(error, "Please enter a file path.");
        return false;
    }

    const std::string temporaryPath = path + ".tmp";
    {
        std::ofstream out(temporaryPath, std::ios::out | std::ios::trunc);
        if (!out)
        {
            SetError(error, "Could not open the temporary project file for writing.");
            return false;
        }

        out << "OPENTAS64_PROJECT 2\n"
            << std::quoted(name) << '\n'
            << frames.size() << ' ' << selectedFrame << '\n';

        for (const auto& frame : frames)
            WriteController(out, frame.controller);

        out.flush();
        if (!out)
        {
            SetError(error, "Writing the project failed. The original file was not replaced.");
            out.close();
            std::remove(temporaryPath.c_str());
            return false;
        }
    }

    // Rename only after the complete temporary file has been written.
    // On platforms where rename cannot replace an existing file, fall back
    // to a remove-and-rename sequence while keeping the temporary file intact.
    if (std::rename(temporaryPath.c_str(), path.c_str()) != 0)
    {
        if (std::remove(path.c_str()) != 0 && errno != ENOENT)
        {
            SetError(error, "Could not replace the target file. Check file permissions.");
            std::remove(temporaryPath.c_str());
            return false;
        }
        if (std::rename(temporaryPath.c_str(), path.c_str()) != 0)
        {
            SetError(error, "Could not move the saved project into place.");
            std::remove(temporaryPath.c_str());
            return false;
        }
    }

    if (error) error->clear();
    return true;
}

bool TasProject::LoadFromFile(const std::string& path, std::string* error)
{
    if (path.empty())
    {
        SetError(error, "Please enter a file path.");
        return false;
    }

    std::ifstream in(path);
    if (!in)
    {
        SetError(error, "Could not open project file.");
        return false;
    }

    std::string magic;
    std::string loadedName;
    int version = 0;
    std::size_t frameCount = 0;
    std::size_t loadedSelectedFrame = 0;

    if (!(in >> magic >> version) || magic != "OPENTAS64_PROJECT" ||
        (version != 1 && version != 2))
    {
        SetError(error, "Unsupported or invalid project file header.");
        return false;
    }

    if (!(in >> std::quoted(loadedName) >> frameCount) ||
        frameCount == 0 || frameCount > kMaximumFrameCount)
    {
        SetError(error, "Invalid project metadata or frame count.");
        return false;
    }

    if (version == 2)
    {
        if (!(in >> loadedSelectedFrame) || loadedSelectedFrame >= frameCount)
        {
            SetError(error, "Invalid selected-frame value in project metadata.");
            return false;
        }
    }

    std::vector<InputFrame> loadedFrames(frameCount);
    for (auto& frame : loadedFrames)
    {
        if (!ReadController(in, frame))
        {
            SetError(error, "Invalid or incomplete controller data in project file.");
            return false;
        }
    }

    std::string trailingData;
    if (in >> trailingData)
    {
        SetError(error, "Unexpected data found after the final frame.");
        return false;
    }

    // Commit parsed data only after the complete file has passed validation.
    name = std::move(loadedName);
    frames = std::move(loadedFrames);
    selectedFrame = loadedSelectedFrame;
    if (error) error->clear();
    return true;
}
