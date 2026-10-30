# NANA_CMAKE_NATIVE_ARCH: optimize nana, and the programs that link it, for the CPU of this computer.
# OFF by default: the binaries run on any CPU of the same architecture, but do not use the newer instructions of this one.
# ON: -march=native (or -mcpu=native, e.g. GCC on PowerPC), if the compiler accepts it. PUBLIC, so that the programs that
# link nana get it too: all the code of a program should be built for the same CPU.

if(NANA_CMAKE_NATIVE_ARCH)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" NANA_COMPILER_HAS_MARCH_NATIVE)
    if(NANA_COMPILER_HAS_MARCH_NATIVE)
        set(nana_native_arch_flag "-march=native")
    else()
        check_cxx_compiler_flag("-mcpu=native" NANA_COMPILER_HAS_MCPU_NATIVE)
        if(NANA_COMPILER_HAS_MCPU_NATIVE)
            set(nana_native_arch_flag "-mcpu=native")
        endif()
    endif()

    if(nana_native_arch_flag)
        target_compile_options(nana PUBLIC ${nana_native_arch_flag})
        message(STATUS "nana: optimized for the CPU of this computer (${nana_native_arch_flag}): "
                       "the binaries may not run on incompatible CPUs. Use -DNANA_CMAKE_NATIVE_ARCH=OFF for portable binaries.")
    else()
        message(STATUS "nana: NANA_CMAKE_NATIVE_ARCH is ON, but the compiler accepts neither -march=native nor -mcpu=native: "
                       "not optimized for the CPU of this computer.")
    endif()
else()
    message(STATUS "nana: not optimized for the CPU of this computer: the binaries are portable, but may lose efficiency. "
                   "Use -DNANA_CMAKE_NATIVE_ARCH=ON to optimize for this CPU (then the binaries may not run on incompatible CPUs).")
endif()
