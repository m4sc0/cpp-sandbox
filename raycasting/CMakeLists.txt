cmake_minimum_required(VERSION 3.10)
project(raycasting)

set(CMAKE_CXX_STANDARD 17)

# Find SDL2 via MSYS2 (it installs a CMake config file)
find_package(SDL2 REQUIRED)

add_executable(raycasting src/main.cpp)
target_include_directories(raycasting PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(raycasting PRIVATE SDL2::SDL2)

