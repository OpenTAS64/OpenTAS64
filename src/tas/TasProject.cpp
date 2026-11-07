#include "TasProject.h"
#include <algorithm>
#include <utility>
TasProject::TasProject(std::string value):name(std::move(value)),frames(1){}
const std::string& TasProject::GetName() const{return name;}
void TasProject::SetName(const std::string& value){name=value;}
std::size_t TasProject::GetFrameCount() const{return frames.size();}
std::size_t TasProject::GetSelectedFrame() const{return selectedFrame;}
bool TasProject::SelectFrame(std::size_t frame){if(frame>=frames.size())return false;selectedFrame=frame;return true;}
void TasProject::EnsureFrame(std::size_t frame){if(frame>=frames.size())frames.resize(frame+1);}
void TasProject::InsertFrame(std::size_t frame){
    frame=std::min(frame,frames.size());
    frames.insert(frames.begin()+static_cast<std::ptrdiff_t>(frame),InputFrame{});
    if(selectedFrame>=frame)++selectedFrame;
    if(selectedFrame>=frames.size())selectedFrame=frames.size()-1;
}
bool TasProject::DeleteFrame(std::size_t frame){
    if(frames.size()<=1||frame>=frames.size())return false;
    frames.erase(frames.begin()+static_cast<std::ptrdiff_t>(frame));
    if(selectedFrame>frame)--selectedFrame;
    else if(selectedFrame==frame&&selectedFrame>=frames.size())selectedFrame=frames.size()-1;
    return true;
}
InputFrame& TasProject::GetFrame(std::size_t frame){EnsureFrame(frame);return frames[frame];}
const InputFrame& TasProject::GetFrame(std::size_t frame) const{return frames.at(frame);}
void TasProject::Clear(){frames.assign(1,InputFrame{});selectedFrame=0;}
