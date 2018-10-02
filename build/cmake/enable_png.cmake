option(NANA_CMAKE_ENABLE_PNG "Enable the use of PNG" OFF)
option(NANA_CMAKE_LIBPNG_FROM_OS "Use libpng from operating system." ON)

# todo: decide - PUBLIC vs PRIVATE

if(NANA_CMAKE_ENABLE_PNG)
    target_compile_definitions(nana PUBLIC NANA_ENABLE_PNG)
    if(NANA_CMAKE_LIBPNG_FROM_OS)
        find_package(PNG)
        if(PNG_FOUND)
            target_include_directories(nana PUBLIC ${PNG_INCLUDE_DIRS})
            target_link_libraries     (nana PUBLIC ${PNG_LIBRARIES})
            target_compile_definitions(nana PUBLIC USE_LIBPNG_FROM_OS ${PNG_DEFINITIONS})
            # target_include_directories  (nana SYSTEM   PUBLIC PNG::PNG)    # ??
            # target_compile_definitions  (nana          PUBLIC USE_LIBPNG_FROM_OS)
        endif()
    else()
        target_link_libraries(nana PUBLIC png)  # provided by nana?
    endif()
endif()