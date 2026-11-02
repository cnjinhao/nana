# Building and using nana 1.8

A minimal guide. A detailed one will follow with the new documentation.

## Requirements

- A C++17 compiler: GCC, Clang, MSVC (Visual Studio 2019 or newer) or MinGW.
- CMake 3.12 or newer.
- Linux: the X11 development packages. On Ubuntu/Debian:

      sudo apt install libx11-dev libxft-dev libxcursor-dev

  Optional: `libpng-dev` and `libjpeg-dev` (PNG and JPEG images), `libasound2-dev` (audio).

## Use nana in your CMake project (recommended)

Put the nana sources next to your project (for example with `git clone https://github.com/cnjinhao/nana.git`) and add
it as a subdirectory:

```cmake
cmake_minimum_required(VERSION 3.12)
project(hello CXX)
add_subdirectory(../nana nana)   # the directory of the nana sources, and a build directory for it
add_executable(hello main.cpp)
target_link_libraries(hello PRIVATE nana)
```

```cpp
#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>

int main()
{
    nana::form fm;
    nana::label lb{ fm, nana::rectangle{ 10, 10, 200, 30 } };
    lb.caption("Hello, nana!");
    fm.show();
    nana::exec();
}
```

Then build as usual:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build

Nana is built as a static library with the same compiler and options as your program.

## Some options

Pass them to CMake as `-D<option>=ON`:

| Option | Effect |
|---|---|
| `NANA_CMAKE_ENABLE_PNG`, `NANA_CMAKE_ENABLE_JPEG` | support of PNG and JPEG images |
| `NANA_CMAKE_NATIVE_ARCH` | optimize for the CPU of this computer (the binaries may not run on other CPUs) |
| `NANA_CMAKE_AUTOMATIC_GUI_TESTING` | register the examples of nana-demo as tests in CTest |

## Examples and tests

The examples of [nana-demo](https://github.com/qPCR4vir/nana-demo) (branch `develop`) are a good start: clone it next to
the nana sources (it expects `../nana`) and build it with CMake as above.
