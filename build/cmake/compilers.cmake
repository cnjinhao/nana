
########### Compilers
#
# Using gcc: gcc 4.8 don't support C++14 and make_unique. You may want to update at least to 4.9.
# gcc 5.3 and 5.4 include filesytem, but you need to add the link flag: -lstdc++fs
#
# In Windows, with CLion Allways check in File/Settings.../toolchains
# You could install MinGW-w64 from the TDM-GCC Compiler Suite for Windows which will update you to gcc 5.1.
# It is posible to follow https://computingabdn.com/softech/mingw-howto-install-gcc-for-windows/
# and install MinGW with gcc 7.1 with has STD_THREADS and fs, from: https://sourceforge.net/projects/mingw-w64/files/
#
# see at end of:  https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dynamic_or_shared.html
#

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") #  AND NOT MINGW??

    target_compile_options(nana PRIVATE  -Wall)

        # todo: set in target property of nana
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG")

    set(THREADS_PREFER_PTHREAD_FLAG ON)               #  todo - test this
    find_package(Threads REQUIRED)
    target_link_libraries(nana PRIVATE Threads::Threads)
    #    target_compile_options(nana PUBLIC -pthread)


    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        if("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
            target_compile_options(nana PUBLIC -I/usr/local/include)
        endif()
    endif()


    # target_link_libraries(nana PRIVATE stdc++fs)    # ??


endif()


if (APPLE AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")                    # APPLE Clang
    target_compile_options(nana PUBLIC -stdlib=libstdc++)
endif ()



if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_compile_options(nana PRIVATE -fmax-errors=3)
endif()

