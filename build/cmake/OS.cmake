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

        if(MSVC_USE_STATIC_RUNTIME)
            foreach(flag
                    CMAKE_C_FLAGS              CMAKE_C_FLAGS_DEBUG             CMAKE_C_FLAGS_RELEASE
                    CMAKE_C_FLAGS_MINSIZEREL   CMAKE_C_FLAGS_RELWITHDEBINFO
                    CMAKE_CXX_FLAGS            CMAKE_CXX_FLAGS_DEBUG           CMAKE_CXX_FLAGS_RELEASE
                    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
                if(${flag} MATCHES "/MD")
                    string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
                endif()
            endforeach()
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
