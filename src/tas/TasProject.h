#pragma once
#include "ControllerState.h"
#include <cstddef>
#include <string>
#include <vector>
class TasProject {
public:
    explicit TasProject(std::string name="Untitled TAS");
    const std::string& GetName() const;
    void SetName(const std::string& value);
    std::size_t GetFrameCount() const;
    std::size_t GetSelectedFrame() const;
    bool SelectFrame(std::size_t frame);
    void EnsureFrame(std::size_t frame);
    void InsertFrame(std::size_t beforeFrame);
    bool DeleteFrame(std::size_t frame);
    InputFrame& GetFrame(std::size_t frame);
    const InputFrame& GetFrame(std::size_t frame) const;
    void Clear();
    bool SaveToFile(const std::string& path, std::string* error = nullptr) const;
    bool LoadFromFile(const std::string& path, std::string* error = nullptr);
private:
    std::string name;
    std::vector<InputFrame> frames;
    std::size_t selectedFrame=0;
};
