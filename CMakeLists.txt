# CMake configuration for Nana
# Author: Andrew Kornilov(https://github.com/ierofant)
# Contributor:
#	Robert Hauck - Enable support for PNG/Freetype
#   Qiangqiang Wu - Add biicode support

if(BIICODE)
    # prepare BII_LIB_SRC
    set(LIB_SRC ${BII_LIB_SRC})

    foreach(cpp ${BII_LIB_SRC})
        if(${cpp} MATCHES "/detail/(win32|linux_X11)/.+$")
            list(APPEND trash_files ${cpp})
        endif()
    endforeach()

    list(REMOVE_ITEM BII_LIB_SRC ${trash_files})
    
    if(WIN32)
        file(GLOB_RECURSE platform_files "*/detail/win32/*")
        list(APPEND BII_LIB_SRC ${platform_files})
    elseif(UNIX)
        file(GLOB_RECURSE platform_files "*/detail/linux_X11/*")
        list(APPEND BII_LIB_SRC ${platform_files})
    else()
        message(FATAL_ERROR "Only Windows and Unix are supported for the moment")
    endif()

    # set compile  flags
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
    endif(CMAKE_COMPILER_IS_GNUCXX)

    # we'll use the default config file so we can iliminate the following macro definitions
    if(MSVC)
        # More MSVC specific compilation flags
		add_definitions(-D_SCL_SECURE_NO_WARNINGS)
		add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif()
    
    add_biicode_targets()
    
    return()
endif()

project(nana)
cmake_minimum_required(VERSION 2.8)

#Select platform automatically
if(WIN32)
	add_definitions(-DNANA_WINDOWS)
	add_definitions(-DPLATFORM_SPEC_HPP=<nana/detail/win32/platform_spec.hpp>)

	#Test if it is MINGW
	if(MINGW)
		add_definitions(-DNANA_MINGW)
		add_definitions(-DSTD_CODECVT_NOT_SUPPORTED)
		option(NANA_THREAD_NOT_SUPPORTED "Use this flag if MinGW version is older than 4.8.1" ON)
		if(NANA_THREAD_NOT_SUPPORTED)
			add_definitions(-DSTD_THREAD_NOT_SUPPORTED)
		endif()
	endif()
endif()
if(UNIX)
	add_definitions(-DNANA_LINUX)
	add_definitions(-DNANA_X11)
	add_definitions(-DPLATFORM_SPEC_HPP=<nana/detail/linux_X11/platform_spec.hpp>)
	add_definitions(-DSTD_CODECVT_NOT_SUPPORTED)
endif()


#Global MSVC definitions
if(WIN32)
	if(MSVC)
		option(WIN32_USE_MP "Set to ON to build nana with the /MP option (Visual Studio 2005 and above)." ON)
		if(WIN32_USE_MP)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		endif()
		
		# More MSVC specific compilation flags
		add_definitions(-D_SCL_SECURE_NO_WARNINGS)
		add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
	endif(MSVC)
endif(WIN32)


#Unicode
option(NANA_UNICODE "Use Unicode Character Set" ON)
if(NANA_UNICODE)
	add_definitions(-DNANA_UNICODE)
	if(WIN32)
		add_definitions(-DUNICODE -D_UNICODE)
	endif()
endif()

#Find PNG
if(UNIX)
    find_package(Freetype)
    if (FREETYPE_FOUND)
        include_directories( ${FREETYPE_INCLUDE_DIRS})
    endif()
endif()

option(NANA_ENABLE_PNG "Enable the use of PNG" ON)
if(NANA_ENABLE_PNG)
    add_definitions(-DNANA_ENABLE_PNG)

    option(NANA_LIBPNG "Use the included libpng" ON)
    if(NANA_LIBPNG)
        add_definitions(-DNANA_LIBPNG)
    else()
        find_package(PNG)
        if (PNG_FOUND)
            include_directories( ${PNG_INCLUDE_DIRS})
        endif()
    endif()
endif()

#Copy our new config.hpp (with removed defines)
execute_process(COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different 
    ${CMAKE_CURRENT_SOURCE_DIR}/build/cmake/config.hpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/nana/)
    
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
endif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

set(NANA_SOURCE_DIR ${CMAKE_SOURCE_DIR}/source)
set(NANA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

include_directories(${NANA_INCLUDE_DIR})
aux_source_directory(${NANA_SOURCE_DIR} NANA_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/detail NANA_DETAIL_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/filesystem NANA_FILESYSTEM_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/audio NANA_AUDIO_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/audio/detail NANA_AUDIO_DETAIL_SOURCE)
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
                            ${NANA_AUDIO_SOURCE}
                            ${NANA_AUDIO_DETAIL_SOURCE}
                            ${NANA_GUI_SOURCE}
                            ${NANA_GUI_DETAIL_SOURCE}
                            ${NANA_GUI_WIDGETS_SOURCE}
                            ${NANA_GUI_WIDGETS_SKELETONS_SOURCE}
                            ${NANA_PAINT_SOURCE}
                            ${NANA_PAINT_DETAIL_SOURCE}
                            ${NANA_SYSTEM_SOURCE}
                            ${NANA_THREADS_SOURCE})                        
install(TARGETS ${PROJECT_NAME}
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib)
install(DIRECTORY ${NANA_INCLUDE_DIR}/nana DESTINATION include)
