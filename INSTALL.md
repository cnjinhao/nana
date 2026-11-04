# Building and using nana 1.8

A minimal guide. A detailed one will follow with the new documentation.

## Requirements

- A C++17 compiler: GCC, Clang (also clang-cl), MSVC (Visual Studio 2019 or newer) or MinGW.
- CMake 3.16 or newer. With Visual Studio, a CMake that knows your version of Visual Studio: the one installed with
  Visual Studio does, or use the Developer Command Prompt (an older CMake may not find the compiler).
- Linux: the X11 development packages. On Ubuntu/Debian:

      sudo apt install libx11-dev libxft-dev libxcursor-dev

  Optional: `libpng-dev` and `libjpeg-dev` (PNG and JPEG images), `libasound2-dev` (audio).

## Use nana in your CMake project (recommended)

Put the nana sources next to your project (for example with `git clone https://github.com/cnjinhao/nana.git`) and add
it as a subdirectory:

```cmake
cmake_minimum_required(VERSION 3.16)
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

With Visual Studio (or any multi-configuration generator) `CMAKE_BUILD_TYPE` is not used: choose the configuration when
building, e.g. `cmake --build build --config Release`.

Nana is built as a static library, from its sources, with the same compiler and options as your program. This is the
simplest way to make sure that the binaries of nana and of your program are compatible: it avoids the many errors that
come from building a library and a program with different settings (C++ standard, compiler version, debug or release,
runtime library...).

With MSVC, nana and your program use the static runtime (`/MT`, `/MTd`) by default; for the DLL runtime pass
`-DMSVC_USE_STATIC_RUNTIME=OFF` (or set `CMAKE_MSVC_RUNTIME_LIBRARY` yourself).

## Some options

Pass them to CMake as `-D<option>=ON`:

| Option | Effect |
|---|---|
| `NANA_CMAKE_ENABLE_PNG`, `NANA_CMAKE_ENABLE_JPEG` | support of PNG and JPEG images; needs libpng / libjpeg (on Windows with MSVC, install them first, e.g. with vcpkg) |
| `NANA_CMAKE_NATIVE_ARCH` | optimize for the CPU of this computer (the binaries may not run on incompatible CPUs) |
| `NANA_CMAKE_AUTOMATIC_GUI_TESTING` | **to test your application** (and how nana-demo runs its examples as tests with CTest): `nana::exec()` becomes `exec(wait = 1, wait_end = 1, test = {})`, which runs `test` in another thread and then closes all the windows, about 2 seconds after the start even without `test`. Use it in a separate test build, not in the build of the executable you deliver. |
| `MSVC_USE_STATIC_RUNTIME` | `ON` by default: static MSVC runtime; `OFF` for the DLL runtime |

## Examples and tests

The examples of [nana-demo](https://github.com/qPCR4vir/nana-demo) (branch `develop`) are a good start: clone it next to
the nana sources (it expects `../nana`) and build it with CMake as above.
