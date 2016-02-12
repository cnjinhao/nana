# CMake configuration for Nana
# Author: Andrew Kornilov(https://github.com/ierofant)
# Contributor:
#	Robert Hauck - Enable support for PNG/Freetype
#   Qiangqiang Wu - Add biicode support
#   Ariel Vina-Rodriguez (qPCR4vir) 

#https://cmake.org/cmake-tutorial/
#https://cmake.org/cmake/help/v3.3/module/CMakeDependentOption.html?highlight=cmakedependentoption
# use CACHE FORCE  or set(ENABLE_MINGW_STD_THREADS_WITH_MEGANZ ON) or delete CMakecache.txt or the entirely build dir
# if your changes don't execute
option(ENABLE_MINGW_STD_THREADS_WITH_MEGANZ "replaced boost.thread with meganz's mingw-std-threads." OFF)
option(ENABLE_PNG "Enable the use of PNG" OFF)
option(LIBPNG_FROM_OS "Use libpng from operating system." ON)
option(ENABLE_JPEG "Enable the use of JPEG" OFF)
option(LIBJPEG_FROM_OS "Use libjpeg from operating system." ON)
option(ENABLE_AUDIO "Enable class audio::play for PCM playback." OFF)
option(CMAKE_VERBOSE_PREPROCESSOR "Show annoying debug messages during compilation." OFF)
option(CMAKE_STOP_VERBOSE_PREPROCESSOR "Stop compilation after showing the annoying debug messages." ON)
option(BUILD_NANA_DEMOS "Build all the demos form the nana_demo repository." OFF)

message("  --->    Running cmake .... ")
	message ( "CMAKE_CXX_COMPILER_ID= " ${CMAKE_CXX_COMPILER_ID})
	message ( "COMPILER_IS_CLANG= " ${COMPILER_IS_CLANG})
	message ( "CMAKE_CXX_FLAGS= "  ${CMAKE_CXX_FLAGS})
	
set(NANA_LINKS)
set(NANA_SOURCES)

project(nana)
cmake_minimum_required(VERSION 2.8)

# Compatibility with CMake 3.1
if(POLICY CMP0054)
  # http://www.cmake.org/cmake/help/v3.1/policy/CMP0054.html
  cmake_policy(SET CMP0054 OLD)
endif()

add_definitions(-DNANA_IGNORE_CONF)

if(WIN32)
    add_definitions(-DWIN32)
    set(BUILD_FreeMe ON)  #"Build FreeMe only on Windows."
    #Global MSVC definitions. You may prefer the hand-tuned sln and projects from the nana repository.
    if(MSVC)
		option(WIN32_USE_MP "Set to ON to build nana with the /MP option (Visual Studio 2005 and above)." ON)
		# ??
		if(WIN32_USE_MP)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		endif(WIN32_USE_MP)
	endif(MSVC)

    if(MINGW)
        if(ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
            add_definitions(-DNANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
        endif(ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
    endif(MINGW)
elseif(WIN32)
    set(BUILD_FreeMe OFF) 
endif(WIN32)

if(APPLE)
    add_definitions(-DAPPLE)
    include_directories(/opt/X11/include/)
	list(APPEND NANA_LINKS -L/opt/X11/lib/  -liconv)
elseif(UNIX)
    add_definitions(-Dlinux)
	message("added -D linux")
endif(APPLE)



if(UNIX)
    list(APPEND NANA_LINKS -lX11  )
    find_package(Freetype)
    if (FREETYPE_FOUND)
        include_directories( ${FREETYPE_INCLUDE_DIRS})
		list(APPEND NANA_LINKS  -lXft )
    endif(FREETYPE_FOUND)
endif(UNIX)

#Find PNG
if(ENABLE_PNG)
    add_definitions(-DNANA_ENABLE_PNG)
    #set(NANA_PNG_LIB "png")
	list(APPEND NANA_LINKS   -lpng )	
    if(LIBPNG_FROM_OS)
        find_package(PNG)
        if (PNG_FOUND)
            include_directories( ${PNG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBPNG_FROM_OS)
        endif(PNG_FOUND)
    endif(LIBPNG_FROM_OS)
#else(ENABLE_PNG)
 #   set(NANA_PNG_LIB "")
endif(ENABLE_PNG)

#Find JPEG
if(ENABLE_JPEG)
    add_definitions(-DNANA_ENABLE_JPEG)
    #set(NANA_JPEG_LIB "jpeg")
	list(APPEND NANA_LINKS   -ljpeg )	
    if(LIBJPEG_FROM_OS)
        find_package(JPEG)
        if (JPEG_FOUND)
            include_directories( ${JPEG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBJPEG_FROM_OS)
        endif(JPEG_FOUND)
    endif(LIBJPEG_FROM_OS)
#else(ENABLE_JPEG)
#    set(NANA_JPEG_LIB "")
endif(ENABLE_JPEG)

if(ENABLE_AUDIO)
    add_definitions(-DNANA_ENABLE_AUDIO)
    if(UNIX)
        find_package(ASOUND)
        if (ASOUND_FOUND)
            include_directories( ${ASOUND_INCLUDE_DIRS})
	        list(APPEND NANA_LINKS   -lasound )				
        else(ASOUND_FOUND)
            message(FATAL_ERROR "libasound is not found")
        endif(ASOUND_FOUND)
    endif(UNIX)
endif(ENABLE_AUDIO)

#Unicode
if(CMAKE_VERBOSE_PREPROCESSOR)
    add_definitions(-DVERBOSE_PREPROCESSOR)
endif(CMAKE_VERBOSE_PREPROCESSOR)


set(NANA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(NANA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

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
endif(APPLE)
							
    #Headers: use INCLUDE_DIRECTORIES
    #   Libraries: use FIND_LIBRARY and link with the result of it (try to avoid LINK_DIRECTORIES
    target_link_libraries(${PROJECT_NAME} ${NANA_LINKS})

                 
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
	
	if (OFF)
		add_executable(file_explorer ../nana-demo/file_explorer.cpp)
		set_property( TARGET file_explorer PROPERTY CXX_STANDARD 14 )
		target_link_libraries(file_explorer ${PROJECT_NAME} )         
		install(TARGETS file_explorer RUNTIME DESTINATION &{DEMO_BIN})
	endif(OFF)

    add_executable(calculator ../nana-demo/calculator.cpp)
    set_property( TARGET calculator PROPERTY CXX_STANDARD 14 )
    target_link_libraries(calculator ${PROJECT_NAME}  )
    install(TARGETS calculator RUNTIME DESTINATION &{DEMO_BIN})

    if (BUILD_FreeMe)
        add_executable(FreeMe ../nana-demo/FreeMe.cpp)
        set_property( TARGET FreeMe PROPERTY CXX_STANDARD 14 )
        target_link_libraries(FreeMe ${PROJECT_NAME}  )
        install(TARGETS FreeMe RUNTIME DESTINATION &{DEMO_BIN})
    endif (BUILD_FreeMe)     

    add_executable(notepad ../nana-demo/notepad.cpp)
    set_property( TARGET notepad PROPERTY CXX_STANDARD 14 )
    target_link_libraries(notepad ${PROJECT_NAME} )# X11 Xft ${NANA_JPEG_LIB} ${NANA_PNG_LIB})
    install(TARGETS notepad RUNTIME DESTINATION &{DEMO_BIN})

	if (OFF)
    add_executable(widget_show ../nana-demo/widget_show.cpp)
    set_property( TARGET widget_show PROPERTY CXX_STANDARD 14 )
    target_link_libraries(widget_show ${PROJECT_NAME})#  X11 Xft ${NANA_JPEG_LIB} ${NANA_PNG_LIB})
    install(TARGETS widget_show RUNTIME DESTINATION &{DEMO_BIN})

    add_executable(widget_show2 ../nana-demo/widget_show2.cpp)
    set_property( TARGET widget_show2 PROPERTY CXX_STANDARD 14 )
    target_link_libraries(widget_show2 ${PROJECT_NAME})#  X11 Xft ${NANA_JPEG_LIB} ${NANA_PNG_LIB})
    install(TARGETS widget_show2 RUNTIME DESTINATION &{DEMO_BIN})
	endif(OFF)
	
    add_executable(a_group_impl ../nana-demo/Examples/a_group_impl.cpp)
    set_property( TARGET a_group_impl PROPERTY CXX_STANDARD 14 )
    target_link_libraries(a_group_impl ${PROJECT_NAME})#  X11 Xft ${NANA_JPEG_LIB} ${NANA_PNG_LIB})

    add_executable(animate-bmp ../nana-demo/Examples/animate-bmp.cpp)
    set_property( TARGET animate-bmp PROPERTY CXX_STANDARD 14 )
    target_link_libraries(animate-bmp ${PROJECT_NAME})#  X11 Xft ${NANA_JPEG_LIB} ${NANA_PNG_LIB})

	if (ENABLE_AUDIO)
		add_executable(audio_player ../nana-demo/Examples/audio_player.cpp)
		set_property( TARGET audio_player PROPERTY CXX_STANDARD 14 )
		target_link_libraries(audio_player ${PROJECT_NAME} ) 
	endif(ENABLE_AUDIO)

    #add_executable(nana_test test.cpp)
    #set_property( TARGET nana_test PROPERTY CXX_STANDARD 14 )
    #target_link_libraries(nana_test ${PROJECT_NAME})#  X11 Xft ${NANA_JPEG_LIB} ${NANA_PNG_LIB})

   #set(NANA_DEMOS_DIR ${CMAKE_SOURCE_DIR}/../nana-demo)
   #set(NANA_EXAMPLES_DIR ${CMAKE_SOURCE_DIR}/../Examples/nana-demo/)
    # https://cmake.org/cmake/help/v3.3/command/file.html?highlight=glob#file
   #file( GLOB_RECURSE DEMO_SOURCES RELATIVE ../nana-demo *.cpp )

   #foreach( demofile ${DEMO_SOURCES} )
   #    string( REPLACE ".cpp" "" demoname ${demofile} )
   #    add_executable( ${demoname} ${demofile} )
   #    set_property( TARGET ${demoname} PROPERTY CXX_STANDARD 14 )
   #    target_link_libraries(${demoname} ${PROJECT_NAME} )# X11 Xft ${NANA_JPEG_LIB} ${NANA_PNG_LIB})
   #endforeach( demofile ${DEMO_SOURCES} )
endif(BUILD_NANA_DEMOS)


# set compile  flags
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")
	else("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
	endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

endif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

    # enable static linkage  
    if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND NOT MINGW)
      #set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
      set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++  -pthread")
	     
    endif ()

    if (APPLE AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
    endif ()

message ( "CMAKE_CXX_COMPILER_ID   = "  ${CMAKE_CXX_COMPILER_ID})
message ( "COMPILER_IS_CLANG       = "  ${COMPILER_IS_CLANG})
message ( "CMAKE_CXX_FLAGS         = "  ${CMAKE_CXX_FLAGS})
message ( "CMAKE_COMPILER_IS_GNUCXX= "  ${CMAKE_COMPILER_IS_GNUCXX})
message ( "CMAKE_EXE_LINKER_FLAGS  = "  ${CMAKE_EXE_LINKER_FLAGS})
message ( "NANA_LINKS              = "  ${NANA_LINKS})
message ( "ENABLE_AUDIO            = "  ${ENABLE_AUDIO})
