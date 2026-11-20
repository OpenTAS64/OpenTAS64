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

# Use the standard C/C++ entry point on every platform. SDL's platform
# entry-point wrapper (SDL2main) is intentionally not linked.
target_compile_definitions(OpenTAS64 PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(OpenTAS64 PRIVATE SDL2::SDL2)

if(WIN32)
    enable_language(RC)
    target_link_libraries(OpenTAS64 PRIVATE ws2_32)

    # Give the built .exe the same icon as the SDL application window.
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(OPEN_TAS64_ICON "${CMAKE_CURRENT_BINARY_DIR}/opentas64.ico")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_windows_icon.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/core/AppIcon.h"
                "${OPEN_TAS64_ICON}"
        RESULT_VARIABLE ICON_GENERATION_RESULT
    )
    if(NOT ICON_GENERATION_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate the OpenTAS64 Windows icon.")
    endif()
    file(TO_CMAKE_PATH "${OPEN_TAS64_ICON}" OPEN_TAS64_ICON_RC_PATH)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/opentas64.rc"
        "IDI_ICON1 ICON \"${OPEN_TAS64_ICON_RC_PATH}\"\\n")
    target_sources(OpenTAS64 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/opentas64.rc")
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
