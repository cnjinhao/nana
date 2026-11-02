########### OS
# https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
# http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system

if(WIN32)
    target_compile_definitions(nana PUBLIC WIN32)    # todo: why not simple test for _WIN32 in code??
    set(CMAKE_DEBUG_POSTFIX "_d")    # ??
    # Global MSVC definitions. You may prefer the hand-tuned sln and projects from the nana repository.
    if(MSVC)
        option(MSVC_USE_MP "Set to ON to build nana with the /MP option (Visual Studio 2005 and above)." ON)
        option(MSVC_USE_STATIC_RUNTIME "Set to ON to build nana with the /MT(d) option." ON)

        # Change the MSVC Compiler flags
        if(MSVC_USE_MP)
            target_compile_options(nana PUBLIC "/MP" )
        endif()

        # Static runtime (/MT, /MTd) by default, for nana and for the programs that use it. For another runtime, set
        # CMAKE_MSVC_RUNTIME_LIBRARY yourself, or this option OFF.
        if(MSVC_USE_STATIC_RUNTIME)
            if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
                # a cache variable: it also reaches the directory that added nana with add_subdirectory()
                set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library")
            endif()
            # the target nana was created before this file
            set_target_properties(nana PROPERTIES MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")

            # A project that requires CMake < 3.15 (policy CMP0091 OLD) has /MD in its flags, and CMAKE_MSVC_RUNTIME_LIBRARY
            # does not change them: change /MD to /MT in the flags of nana and of the directory that added it.
            set(nana_runtime_in_flags FALSE)
            foreach(flag
                    CMAKE_C_FLAGS              CMAKE_C_FLAGS_DEBUG             CMAKE_C_FLAGS_RELEASE
                    CMAKE_C_FLAGS_MINSIZEREL   CMAKE_C_FLAGS_RELWITHDEBINFO
                    CMAKE_CXX_FLAGS            CMAKE_CXX_FLAGS_DEBUG           CMAKE_CXX_FLAGS_RELEASE
                    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
                if(${flag} MATCHES "/MD")
                    set(nana_runtime_in_flags TRUE)
                    string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
                    if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
                        set(${flag} "${${flag}}" PARENT_SCOPE)
                    endif()
                endif()
            endforeach()
            if(nana_runtime_in_flags)
                message(WARNING "nana: your project requires a CMake older than 3.15, so its flags select the MSVC runtime. "
                                "nana changed /MD to /MT in its flags and in those of the directory that added it; other "
                                "directories keep /MD. Use cmake_minimum_required(VERSION 3.16) or newer.")
            endif()
        endif()
    endif()

    if(MINGW)
        if(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)      # deprecated ?????
            target_compile_definitions(nana PUBLIC STD_THREAD_NOT_SUPPORTED
                                            PUBLIC NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ  )
        endif()
    endif()
endif()

if(APPLE)
    target_compile_definitions(nana PUBLIC APPLE)      # ??? not added by compilers? use  __APPLE__  ?
    target_include_directories(nana PUBLIC /opt/X11/include/)
    target_link_libraries(nana PRIVATE iconv)
    set(ENABLE_AUDIO OFF)
endif()

if(UNIX)

    find_package(X11 REQUIRED)            # X11  - todo test PRIVATE
    target_link_libraries(nana
            PUBLIC ${X11_LIBRARIES}
            PUBLIC ${X11_Xft_LIB}
            )
    target_include_directories(nana SYSTEM
            PUBLIC ${X11_Xft_INCLUDE_PATH}
            PUBLIC ${X11_INCLUDE_DIR}
            )

    find_package(Freetype)                # Freetype - todo test PRIVATE
    if (FREETYPE_FOUND)
        find_package(Fontconfig REQUIRED)
        target_include_directories(nana SYSTEM
                PUBLIC ${FREETYPE_INCLUDE_DIRS}
                PUBLIC ${FONTCONFIG_INCLUDE_DIR}
                )
        target_link_libraries(nana
                PUBLIC ${FREETYPE_LIBRARIES}
                PUBLIC ${FONTCONFIG_LIBRARIES}
                )
    endif(FREETYPE_FOUND)
endif(UNIX)
