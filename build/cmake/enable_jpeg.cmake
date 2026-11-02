option(NANA_CMAKE_ENABLE_JPEG "Enable the use of JPEG" OFF)
option(NANA_CMAKE_LIBJPEG_FROM_OS "Use libjpeg from operating system." ON)
option(JPEG_HAVE_BOOLEAN "Defining HAVE_BOOLEAN before including jpeglib.h" OFF)

# todo: decide - PUBLIC vs PRIVATE

if(NANA_CMAKE_ENABLE_JPEG)
    target_compile_definitions(nana PUBLIC NANA_ENABLE_JPEG)
    if(NANA_CMAKE_LIBJPEG_FROM_OS)
        find_package(JPEG)
        if(JPEG_FOUND)
            target_include_directories(nana PUBLIC  ${JPEG_INCLUDE_DIRS})
            target_link_libraries     (nana PUBLIC  ${JPEG_LIBRARIES})
            target_compile_definitions(nana PUBLIC  USE_LIBJPEG_FROM_OS)
        else()
            # otherwise the build fails later, at jpeglib.h
            message(FATAL_ERROR "nana: NANA_CMAKE_ENABLE_JPEG is ON, but libjpeg was not found. Install it (on Windows "
                                "with MSVC, for example with vcpkg) or set NANA_CMAKE_ENABLE_JPEG=OFF.")
        endif()
    else()
        target_compile_definitions(nana PUBLIC -ljpeg)
    endif()
    if(JPEG_HAVE_BOOLEAN)
        # ... Defining HAVE_BOOLEAN before including jpeglib.h should make it work...
        target_compile_definitions(nana PUBLIC HAVE_BOOLEAN)
    endif()

endif()