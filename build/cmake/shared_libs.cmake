
option(BUILD_SHARED_LIBS "Compile nana as a shared library." OFF)
option(NANA_STATIC_STDLIB "Link nana statically to C++ standard library" ON)

if(BUILD_SHARED_LIBS)   # todo test

    if(WIN32)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
        if(MSVC)
            set(DLLTOOL OFF)
        else()
            # mingw: If dlltool is found the def and lib file will be created
            find_program (DLLTOOL dlltool)
            if(NOT DLLTOOL)
                message(WARNING "dlltool not found. Skipping import library generation.")
            endif()
        endif()
        if(DLLTOOL)
            #generate the lib and def files needed by msvc
            set_target_properties (nana   PROPERTIES
                    OUTPUT_NAME         nana
                    ARCHIVE_OUTPUT_NAME nana
                    LINK_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,--output-def=${CMAKE_CURRENT_BINARY_DIR}/libnana.def"
                    )

            add_custom_command(TARGET nana POST_BUILD
                    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                    COMMAND echo "       Generating import library"
                    COMMAND "${DLLTOOL}" --dllname    "libnana.dll"
                    --input-def  "libnana.def"
                    --output-lib "libnana.lib")

            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libnana.def"
                          "${CMAKE_CURRENT_BINARY_DIR}/libnana.lib" DESTINATION lib)
        endif()
    endif()
endif()

if(NANA_STATIC_STDLIB)
    target_link_libraries(nana
        PUBLIC
            $<$<CXX_COMPILER_ID:GNU>:-static-libgcc -static-libstdc++>
            $<$<CXX_COMPILER_ID:Clang>:-static-libgcc -static-libstdc++>
            )
endif()

