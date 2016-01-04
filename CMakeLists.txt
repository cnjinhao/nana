# CMake configuration for Nana
# Author: Andrew Kornilov(https://github.com/ierofant)
# Contributor:
#	Robert Hauck - Enable support for PNG/Freetype
#   Qiangqiang Wu - Add biicode support
#   Ariel Vina-Rodriguez (qPCR4vir) 

#https://cmake.org/cmake-tutorial/
#https://cmake.org/cmake/help/v3.3/module/CMakeDependentOption.html?highlight=cmakedependentoption
# use CACHE FORCE  or set(USE_meganz_mingw_std_threads ON) or delete CMakecache.txt or the entirely build dir
# if your changes don't execute
option(USE_meganz_mingw_std_threads "replaced boost.thread with meganz's mingw-std-threads." OFF)
option(USE_UNICODE "Use Unicode Character Set" ON)
option(ENABLE_PNG "Enable the use of PNG" OFF)
option(LIBPNG_FROM_OS "Use libpng from operating system." ON)
option(ENABLE_JPEG "Enable the use of JPEG" OFF)
option(LIBJPEG_FROM_OS "Use libjpeg from operating system." ON)
option(CMAKE_VERBOSE_PREPROCESSOR "Show annoying debug messages during compilation." OFF)
option(CMAKE_STOP_VERBOSE_PREPROCESSOR "Stop compilation after showing the annoying debug messages." ON)
option(BUILD_NANA_DEMOS "Build all the demos form the nana_demo repository." OFF)
option(CMAKE_STD_make_unique_NOT_SUPPORTED "Add support for make_unique<>()." OFF)
#set(CMAKE_STD_make_unique_NOT_SUPPORTED OFF)

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
endif(BIICODE)

project(nana)
cmake_minimum_required(VERSION 2.8)


if(WIN32)
    add_definitions(-DWIN32)

    #Global MSVC definitions
    if(MSVC)
		option(WIN32_USE_MP "Set to ON to build nana with the /MP option (Visual Studio 2005 and above)." ON)
		# ??
		if(WIN32_USE_MP)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		endif(WIN32_USE_MP)
	endif(MSVC)

    if(MINGW)
        add_definitions(-DMINGW)
        #Find PNG
        if(USE_meganz_mingw_std_threads)
            add_definitions(-DUSE_github_com_meganz_mingw_std_threads)
            # remove_definitions()
        endif(USE_meganz_mingw_std_threads)
    endif(MINGW)

endif(WIN32)


#Unicode
if(USE_UNICODE)
    add_definitions(-DNANA_UNICODE)
endif(USE_UNICODE)

if(APPLE)
    add_definitions(-DAPPLE)
    include_directories(/opt/X11/include/)
elseif(UNIX)
    add_definitions(-Dlinux)
endif(APPLE)



if(UNIX)
    find_package(Freetype)
    if (FREETYPE_FOUND)
        include_directories( ${FREETYPE_INCLUDE_DIRS})
    endif(FREETYPE_FOUND)
endif(UNIX)

#Find PNG
if(ENABLE_PNG)
    add_definitions(-DNANA_ENABLE_PNG)
    if(LIBPNG_FROM_OS)
        find_package(PNG)
        if (PNG_FOUND)
            include_directories( ${PNG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBPNG_FROM_OS)
        endif(PNG_FOUND)
    endif(LIBPNG_FROM_OS)
endif(ENABLE_PNG)

#Find JPEG
if(ENABLE_JPEG)
    add_definitions(-DNANA_ENABLE_JPEG)
    if(LIBJPEG_FROM_OS)
        find_package(JPEG)
        if (JPEG_FOUND)
            include_directories( ${JPEG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBJPEG_FROM_OS)
        endif(JPEG_FOUND)
    endif(LIBJPEG_FROM_OS)
endif(ENABLE_JPEG)



#Unicode
if(CMAKE_VERBOSE_PREPROCESSOR)
    add_definitions(-DVERBOSE_PREPROCESSOR)
endif(CMAKE_VERBOSE_PREPROCESSOR)

# STD_make_unique_NOT_SUPPORTED
if(CMAKE_STD_make_unique_NOT_SUPPORTED)
    add_definitions(-DSTD_make_unique_NOT_SUPPORTED)
endif(CMAKE_STD_make_unique_NOT_SUPPORTED)


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

#To show .h files in Visual Studio, add them to the list of sources in add_executable / add_library
#and Use SOURCE_GROUP if all your sources are in the same directory

if(NOT APPLE)
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
endif (NOT APPLE)   
                            

if(APPLE)
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
    #Headers: use INCLUDE_DIRECTORIES
    #   Libraries: use FIND_LIBRARY and link with the result of it (try to avoid LINK_DIRECTORIES
    target_link_libraries(${PROJECT_NAME} -L/opt/X11/lib/ -lX11 -lXft -lpng -liconv)

endif(APPLE)
                 
install(TARGETS ${PROJECT_NAME}
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib)
# ??
install(DIRECTORY ${NANA_INCLUDE_DIR}/nana DESTINATION include)

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

if (BUILD_NANA_DEMOS)
    set (CMAKE_INSTALL_PREFIX ${DEMO_BIN})
    set(DEMO_BIN ${NANA_SOURCE_DIR}../nana-demo/bin)
    set(CMAKE_INSTALL_PREFIX )
    add_executable(file_explorer ../nana-demo/file_explorer.cpp)
    set_property( TARGET file_explorer PROPERTY CXX_STANDARD 14 )
    target_link_libraries(file_explorer ${PROJECT_NAME})
    install(TARGETS file_explorer RUNTIME DESTINATION &{DEMO_BIN})

    add_executable(calculator ../nana-demo/calculator.cpp)
    set_property( TARGET calculator PROPERTY CXX_STANDARD 14 )
    target_link_libraries(calculator ${PROJECT_NAME})
    install(TARGETS calculator RUNTIME DESTINATION &{DEMO_BIN})

    add_executable(FreeMe ../nana-demo/FreeMe.cpp)
    set_property( TARGET FreeMe PROPERTY CXX_STANDARD 14 )
    target_link_libraries(FreeMe ${PROJECT_NAME})
    install(TARGETS FreeMe RUNTIME DESTINATION &{DEMO_BIN})

    add_executable(notepad ../nana-demo/notepad.cpp)
    set_property( TARGET notepad PROPERTY CXX_STANDARD 14 )
    target_link_libraries(notepad ${PROJECT_NAME})
    install(TARGETS notepad RUNTIME DESTINATION &{DEMO_BIN})

    add_executable(widget_show ../nana-demo/widget_show.cpp)
    set_property( TARGET widget_show PROPERTY CXX_STANDARD 14 )
    target_link_libraries(widget_show ${PROJECT_NAME})
    install(TARGETS widget_show RUNTIME DESTINATION &{DEMO_BIN})

    add_executable(widget_show2 ../nana-demo/widget_show2.cpp)
    set_property( TARGET widget_show2 PROPERTY CXX_STANDARD 14 )
    target_link_libraries(widget_show2 ${PROJECT_NAME})
    install(TARGETS widget_show2 RUNTIME DESTINATION &{DEMO_BIN})

    add_executable(a_group_impl ../nana-demo/Examples/a_group_impl.cpp)
    set_property( TARGET a_group_impl PROPERTY CXX_STANDARD 14 )
    target_link_libraries(a_group_impl ${PROJECT_NAME})

    add_executable(animate-bmp ../nana-demo/Examples/animate-bmp.cpp)
    set_property( TARGET animate-bmp PROPERTY CXX_STANDARD 14 )
    target_link_libraries(animate-bmp ${PROJECT_NAME})

    add_executable(audio_player ../nana-demo/Examples/audio_player.cpp)
    set_property( TARGET audio_player PROPERTY CXX_STANDARD 14 )
    target_link_libraries(audio_player ${PROJECT_NAME})


    #add_executable(nana_test test.cpp)
    #set_property( TARGET nana_test PROPERTY CXX_STANDARD 14 )
    #target_link_libraries(nana_test ${PROJECT_NAME})

   #set(NANA_DEMOS_DIR ${CMAKE_SOURCE_DIR}/../nana-demo)
   #set(NANA_EXAMPLES_DIR ${CMAKE_SOURCE_DIR}/../Examples/nana-demo/)
    # https://cmake.org/cmake/help/v3.3/command/file.html?highlight=glob#file
   #file( GLOB_RECURSE DEMO_SOURCES RELATIVE ../nana-demo *.cpp )

   #foreach( demofile ${DEMO_SOURCES} )
   #    string( REPLACE ".cpp" "" demoname ${demofile} )
   #    add_executable( ${demoname} ${demofile} )
   #    set_property( TARGET ${demoname} PROPERTY CXX_STANDARD 14 )
   #    target_link_libraries(${demoname} ${PROJECT_NAME})
   #endforeach( demofile ${DEMO_SOURCES} )
endif(BUILD_NANA_DEMOS)

