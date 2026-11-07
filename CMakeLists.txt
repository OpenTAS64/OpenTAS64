cmake_minimum_required(VERSION 3.16)
project(OpenTAS64 VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
find_package(SDL2 REQUIRED)

add_executable(OpenTAS64
    src/main.cpp
    src/core/App.cpp
    src/network/TcpClient.cpp
    src/tas/TasProject.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_sdl2.cpp
    third_party/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(OpenTAS64 PRIVATE
    src
    third_party/imgui
    third_party/imgui/backends
)
if(TARGET SDL2::SDL2main)
    target_link_libraries(OpenTAS64 PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(OpenTAS64 PRIVATE SDL2::SDL2)

if(WIN32)
    target_link_libraries(OpenTAS64 PRIVATE ws2_32)
endif()

add_subdirectory(plugins/mupen64plus-input)

if(BUILD_TESTING)
    add_executable(TasProjectTests
        tests/TasProjectTests.cpp
        src/tas/TasProject.cpp
    )
    target_include_directories(TasProjectTests PRIVATE src)
    add_test(NAME TasProjectTests COMMAND TasProjectTests)
endif()
