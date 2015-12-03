# CMake configuration for Nana
# Author: Andrew Kornilov(https://github.com/ierofant)
# Contributor:
#	Robert Hauck - Enable support for PNG/Freetype
#   Qiangqiang Wu - Add biicode support

# set compile  flags
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
endif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")


# move this to the end ??
if(BIICODE)
    # prepare BII_LIB_SRC
    set(LIB_SRC ${BII_LIB_SRC})

    add_biicode_targets()
    
    return()
endif()

project(nana)
cmake_minimum_required(VERSION 2.8)


#Global MSVC definitions
if(WIN32)
	if(MSVC)
		option(WIN32_USE_MP "Set to ON to build nana with the /MP option (Visual Studio 2005 and above)." ON)
		# ??
		if(WIN32_USE_MP)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		endif()
		
	endif(MSVC)
endif(WIN32)


#Unicode
option(USE_UNICODE "Use Unicode Character Set")
if(USE_UNICODE)
    add_definitions(-DNANA_UNICODE)
endif()

if(APPLE)
    add_definitions(-DAPPLE)
    include_directories(/opt/X11/include/)
elseif(UNIX)
    add_definitions(-Dlinux)
endif()



if(UNIX)
    find_package(Freetype)
    if (FREETYPE_FOUND)
        include_directories( ${FREETYPE_INCLUDE_DIRS})
    endif()
endif()

if(WIN32)
    add_definitions(-DWIN32)
    if(MINGW)
        add_definitions(-DMINGW)
    endif()
endif()

#Find PNG
option(ENABLE_PNG "Enable the use of PNG")
option(LIBPNG_FROM_OS "Use libpng from operating system.")
if(ENABLE_PNG)
    add_definitions(-DNANA_ENABLE_PNG)
    if(LIBPNG_FROM_OS)
        find_package(PNG)
        if (PNG_FOUND)
            include_directories( ${PNG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBPNG_FROM_OS)
        endif()
    endif()
endif()

#Find JPEG
option(ENABLE_JPEG "Enable the use of JPEG")
option(LIBJPEG_FROM_OS "Use libjpeg from operating system.")
if(ENABLE_JPEG)
    add_definitions(-DNANA_ENABLE_JPEG)
    if(LIBJPEG_FROM_OS)
        find_package(JPEG)
        if (JPEG_FOUND)
            include_directories( ${JPEG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBJPEG_FROM_OS)
        endif()
    endif()
endif()



set(NANA_SOURCE_DIR ${CMAKE_SOURCE_DIR}/source)
set(NANA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

include_directories(${NANA_INCLUDE_DIR})
aux_source_directory(${NANA_SOURCE_DIR} NANA_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/detail NANA_DETAIL_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/filesystem NANA_FILESYSTEM_SOURCE)
if(NOT APPLE)
aux_source_directory(${NANA_SOURCE_DIR}/audio NANA_AUDIO_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/audio/detail NANA_AUDIO_DETAIL_SOURCE)
endif()
aux_source_directory(${NANA_SOURCE_DIR}/gui NANA_GUI_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/gui/detail NANA_GUI_DETAIL_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/gui/widgets NANA_GUI_WIDGETS_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/gui/widgets/skeletons NANA_GUI_WIDGETS_SKELETONS_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/paint NANA_PAINT_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/paint/detail NANA_PAINT_DETAIL_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/system NANA_SYSTEM_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/threads NANA_THREADS_SOURCE)

add_library(${PROJECT_NAME} ${NANA_SOURCE}
                            ${NANA_DETAIL_SOURCE}
                            ${NANA_FILESYSTEM_SOURCE}
#if(NOT APPLE)
                            ${NANA_AUDIO_SOURCE}
                            ${NANA_AUDIO_DETAIL_SOURCE}
#endif
                            ${NANA_GUI_SOURCE}
                            ${NANA_GUI_DETAIL_SOURCE}
                            ${NANA_GUI_WIDGETS_SOURCE}
                            ${NANA_GUI_WIDGETS_SKELETONS_SOURCE}
                            ${NANA_PAINT_SOURCE}
                            ${NANA_PAINT_DETAIL_SOURCE}
                            ${NANA_SYSTEM_SOURCE}
                            ${NANA_THREADS_SOURCE})       

#if(APPLE)
target_link_libraries(${PROJECT_NAME} -L/opt/X11/lib/ -lX11 -lXft -lpng -liconv)
#endif()
                 
install(TARGETS ${PROJECT_NAME}
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib)
install(DIRECTORY ${NANA_INCLUDE_DIR}/nana DESTINATION include)

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

add_executable(nana_test test.cpp)
set_property( TARGET nana_test PROPERTY CXX_STANDARD 14 )
target_link_libraries(nana_test ${PROJECT_NAME})
