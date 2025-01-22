cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Name abrams_tiny_term)

project(${Name})

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

set(Headers
    include/ascii_font.h
)

set(Sources
    src/main.cpp
    src/ascii_font.cpp
)

add_executable(${Name} ${Headers} ${Sources})

target_link_libraries(${Name} PRIVATE SDL3::SDL3)

enable_testing()
