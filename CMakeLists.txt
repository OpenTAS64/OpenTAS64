cmake_minimum_required(VERSION 3.16)

project(OpenTAS64 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(OpenTAS64
    src/main.cpp
    src/App.cpp
)

target_link_libraries(OpenTAS64 PRIVATE SDL2::SDL2)