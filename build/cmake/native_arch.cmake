# NANA_CMAKE_NATIVE_ARCH: optimize nana, and the programs that link it, for all CPUs compatible with the one in this computer.
# OFF by default: the binaries also run on a few other CPUs of the same architecture (mostly older ones), but do not use the newer instructions of this one.
# ON: -march=native (or -mcpu=native, e.g. GCC on PowerPC), if the compiler accepts it. PUBLIC, so that the programs that
# link nana get it too: all the code of a program is then built for the same compatible CPUs.

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
        message(STATUS "nana: optimized for all CPUs compatible with the one in this computer (${nana_native_arch_flag}): "
                       "the binaries may not run on a few incompatible CPUs. Use -DNANA_CMAKE_NATIVE_ARCH=OFF for portable binaries.")
    else()
        message(STATUS "nana: NANA_CMAKE_NATIVE_ARCH is ON, but the compiler accepts neither -march=native nor -mcpu=native: "
                       "generic code: not optimized.")
    endif()
else()
    message(STATUS "nana: not optimized for the CPU of this computer: the binaries also run on some exotic or older CPUs, but may be less efficient. "
                   "Use -DNANA_CMAKE_NATIVE_ARCH=ON to optimize for all CPUs compatible with this one (the binaries may then not run on a few incompatible CPUs).")
endif()
