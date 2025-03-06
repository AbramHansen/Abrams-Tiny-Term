# Abram's Tiny Term

## About

A hardware accelerated terminal emulator for Linux.
Designed for small resolutions and raster fonts.

[MIT Licence](/LICENCE)

Created by Abram Hansen

## Install Instructions

1. Prerequisites:
    * SDL3 installed [Instructions](https://github.com/libsdl-org/SDL/blob/main/docs/README-cmake.md)
    * A version of CMake at least as new as the version listed at the top of CMakeLists.txt
    * A build system such as make or ninja
    * A C++ compiler capable of compiling the C++ version listed in CMakeLists.txt such as gcc or clang
2. Once in the base directory for the project run: `mkdir build && cd build`
3. Run `cmake -DCMAKE_BUILD_TYPE=Release ..`
4. Run `cmake --build .`
5. Run `sudo cmake -- install .`
6. Run `abrams_tiny_term` to launch

## Uninstall Instructions

Run `sudo abrams_tiny_term --uninstall`
