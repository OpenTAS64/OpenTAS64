#pragma once
#include <cstdint>
struct ControllerState {
    bool a=false,b=false,z=false,start=false;
    bool dUp=false,dDown=false,dLeft=false,dRight=false;
    bool l=false,r=false,cUp=false,cDown=false,cLeft=false,cRight=false;
    std::int8_t stickX=0, stickY=0;
};
struct InputFrame { ControllerState controller; };
