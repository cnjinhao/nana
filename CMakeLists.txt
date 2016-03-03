# CMake configuration for Nana
# Author: Andrew Kornilov(https://github.com/ierofant)
# Contributors:
#   Jinhao	
#   Robert Hauck - Enable support for PNG/Freetype
#   Qiangqiang Wu - Add biicode support
#   Ariel Vina-Rodriguez (qPCR4vir) 

#https://cmake.org/cmake-tutorial/
#https://cmake.org/cmake/help/v3.3/module/CMakeDependentOption.html?highlight=cmakedependentoption
# use CACHE FORCE  or set(ENABLE_MINGW_STD_THREADS_WITH_MEGANZ ON) or delete CMakecache.txt or the entirely build dir
# if your changes don't execute

#  It seems that project() defines essential system variables like CMAKE_FIND_LIBRARY_PREFIXES.
#  https://bbs.archlinux.org/viewtopic.php?id=84967
project(nana)
cmake_minimum_required(VERSION 2.8)

set(NANA_LINKS)

option(NANA_CMAKE_INSTALL_INCLUDES "Install nana includes when compile the library" ON)
option(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ "replaced boost.thread with meganz's mingw-std-threads." OFF)
option(NANA_CMAKE_ENABLE_PNG "Enable the use of PNG" OFF)
option(NANA_CMAKE_LIBPNG_FROM_OS "Use libpng from operating system." ON)
option(NANA_CMAKE_ENABLE_JPEG "Enable the use of JPEG" OFF)
option(NANA_CMAKE_LIBJPEG_FROM_OS "Use libjpeg from operating system." ON)
option(NANA_CMAKE_ENABLE_AUDIO "Enable class audio::play for PCM playback." OFF)
option(NANA_CMAKE_VERBOSE_PREPROCESSOR "Show annoying debug messages during compilation." OFF)
option(NANA_CMAKE_STOP_VERBOSE_PREPROCESSOR "Stop compilation after showing the annoying debug messages." ON)
option(NANA_CMAKE_BUILD_DEMOS "Build all the demos form the nana_demo repository." OFF)
option(NANA_CMAKE_INCLUDE_EXPERIMENTAL_DEMOS "" ON)
option(NANA_CMAKE_AUTOMATIC_GUI_TESTING "Activate automatic GUI testing?" OFF)
option(NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING "Add defoult automatic GUI test?" OFF)
option(NANA_CMAKE_BUILD_FreeMe "Build FreeMe (currently broken)?" OFF)


# The ISO C++ File System Technical Specification (ISO-TS, or STD) is optional.
#              http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf
# This is not a workaround, but an user option.
# The library maybe available in the std library in use or from Boost (almost compatible)
#              http://www.boost.org/doc/libs/1_60_0/libs/filesystem/doc/index.htm
# or you can choose to use the (partial, but functional) implementation provided by nana.
# If you include the file <nana/filesystem/filesystem_selector.hpp>
# The selected option will be set by nana into std::experimental::filesystem
# By default Nana will try to use the STD. If not available will try 
# to use boost if available. Nana own implementation will be use only none of them are available.
# You can change that default if you change one of the following 
# (please don't define more than one of the _XX_FORCE options):
option(NANA_CMAKE_FIND_BOOST_FILESYSTEM "Search: Is Boost filesystem available?" ON)
option(NANA_CMAKE_NANA_FILESYSTEM_FORCE "Force nana filesystem over ISO and boost?" OFF)
option(NANA_CMAKE_STD_FILESYSTEM_FORCE "Use of STD filesystem?(a compilation error will ocurre if not available)" OFF)
option(NANA_CMAKE_BOOST_FILESYSTEM_FORCE "Force use of Boost filesystem if available (over STD)?" OFF)
# cmake will find the package self, if don't works set the following (please find the correct values):
#option(NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT "Where to find <boost/filesystem.hpp>?" "../")
#option(NANA_CMAKE_BOOST_FILESYSTEM_LIB "Flag for the compiler to link: " "-lboost/fs")
#include_directories("${NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT}")
#list(APPEND NANA_LINKS "${NANA_CMAKE_BOOST_FILESYSTEM_LIB}")


if (NANA_CMAKE_NANA_FILESYSTEM_FORCE)
    add_definitions(-DNANA_FILESYSTEM_FORCE)
	
elseif (NANA_CMAKE_STD_FILESYSTEM_FORCE)
    add_definitions(-DSTD_FILESYSTEM_FORCE)
	
elseif (NANA_CMAKE_FIND_BOOST_FILESYSTEM OR NANA_CMAKE_BOOST_FILESYSTEM_FORCE)

	if (NANA_CMAKE_BOOST_FILESYSTEM_FORCE)
        add_definitions(-DNANA_BOOST_FILESYSTEM_FORCE)
	endif(NANA_CMAKE_BOOST_FILESYSTEM_FORCE)
	
    # https://cmake.org/cmake/help/git-master/module/FindBoost.html
	# Implicit dependencies such as Boost::filesystem requiring Boost::system will be automatically detected and satisfied, 
	# even if system is not specified when using find_package and if Boost::system is not added to target_link_libraries. 
	# If using Boost::thread, then Thread::Thread will also be added automatically.
    find_package(Boost COMPONENTS filesystem)
	if (Boost_FOUND)
        add_definitions(-DNANA_BOOST_FILESYSTEM_AVAILABLE)
		include_directories(SYSTEM "${Boost_INCLUDE_DIR}")
		list(APPEND NANA_LINKS "${Boost_LIBRARIES}")
	endif (Boost_FOUND)
	
	set(Boost_USE_STATIC_LIBS ON)
	set(Boost_USE_STATIC_RUNTIME ON)  # ??
	#set(Boost_USE_MULTITHREADED ON)
	
endif (NANA_CMAKE_NANA_FILESYSTEM_FORCE)



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
        if(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
            add_definitions(-DNANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
        endif(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    add_definitions(-DAPPLE)
    include_directories(/opt/X11/include/)
	list(APPEND NANA_LINKS -L/opt/X11/lib/  -liconv)
    set(ENABLE_AUDIO OFF)
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
if(NANA_CMAKE_ENABLE_PNG)
    add_definitions(-DNANA_ENABLE_PNG)
    #set(NANA_PNG_LIB "png")
	list(APPEND NANA_LINKS   -lpng )	
    if(NANA_CMAKE_LIBPNG_FROM_OS)
        find_package(PNG)
        if (PNG_FOUND)
            include_directories( ${PNG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBPNG_FROM_OS)
        endif(PNG_FOUND)
    endif(NANA_CMAKE_LIBPNG_FROM_OS)
endif(NANA_CMAKE_ENABLE_PNG)

#Find JPEG
if(NANA_CMAKE_ENABLE_JPEG)
    add_definitions(-DNANA_ENABLE_JPEG)
    #set(NANA_JPEG_LIB "jpeg")
	list(APPEND NANA_LINKS   -ljpeg )	
    if(NANA_CMAKE_LIBJPEG_FROM_OS)
        find_package(JPEG)
        if (JPEG_FOUND)
            include_directories( ${JPEG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBJPEG_FROM_OS)
        endif(JPEG_FOUND)
    endif(NANA_CMAKE_LIBJPEG_FROM_OS)
endif(NANA_CMAKE_ENABLE_JPEG)

if(NANA_CMAKE_ENABLE_AUDIO)
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
endif(NANA_CMAKE_ENABLE_AUDIO)

if (NANA_CMAKE_BUILD_FreeMe)
    add_definitions(-DBUILD_FreeMe)
endif (NANA_CMAKE_BUILD_FreeMe)


if(NANA_CMAKE_AUTOMATIC_GUI_TESTING)
    add_definitions(-DNANA_AUTOMATIC_GUI_TESTING)

    if(NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING)
        add_definitions(-DNANA_ADD_DEF_AUTOMATIC_GUI_TESTING)
    endif(NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING)

endif(NANA_CMAKE_AUTOMATIC_GUI_TESTING)



if(NANA_CMAKE_VERBOSE_PREPROCESSOR)
    add_definitions(-DVERBOSE_PREPROCESSOR)
endif(NANA_CMAKE_VERBOSE_PREPROCESSOR)


set(NANA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(NANA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${NANA_INCLUDE_DIR})
aux_source_directory(${NANA_SOURCE_DIR} NANA_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/detail NANA_DETAIL_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/filesystem NANA_FILESYSTEM_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/gui NANA_GUI_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/gui/detail NANA_GUI_DETAIL_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/gui/widgets NANA_GUI_WIDGETS_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/gui/widgets/skeletons NANA_GUI_WIDGETS_SKELETONS_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/paint NANA_PAINT_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/paint/detail NANA_PAINT_DETAIL_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/system NANA_SYSTEM_SOURCE)
aux_source_directory(${NANA_SOURCE_DIR}/threads NANA_THREADS_SOURCE)
if(NANA_CMAKE_ENABLE_AUDIO)
    aux_source_directory(${NANA_SOURCE_DIR}/audio NANA_AUDIO_SOURCE)
    aux_source_directory(${NANA_SOURCE_DIR}/audio/detail NANA_AUDIO_DETAIL_SOURCE)
endif()

#To show .h files in Visual Studio, add them to the list of sources in add_executable / add_library
#and Use SOURCE_GROUP if all your sources are in the same directory

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

    #  Headers: use INCLUDE_DIRECTORIES
    #  Libraries: use FIND_LIBRARY and link with the result of it (try to avoid LINK_DIRECTORIES)
    target_link_libraries(${PROJECT_NAME} ${NANA_LINKS})

                 
install(TARGETS ${PROJECT_NAME}
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib)

# Install include directories too.
if(NANA_CMAKE_INSTALL_INCLUDES)
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/nana
			DESTINATION include)
endif(NANA_CMAKE_INSTALL_INCLUDES)

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

# TODO: move this nana-demo section to the nana demo repository, and here only include that cmake file

if (NANA_CMAKE_BUILD_DEMOS)

	if(NANA_CMAKE_AUTOMATIC_GUI_TESTING)
	     add_definitions(-DNANA_AUTOMATIC_GUI_TESTING)
         enable_testing ()
	endif(NANA_CMAKE_AUTOMATIC_GUI_TESTING)

	if(NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING)
	     add_definitions(-DNANA_ADD_DEF_AUTOMATIC_GUI_TESTING)
	endif(NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING)
	
    set (demos     helloworld_demo notepad        )

    foreach ( demo ${demos})
        add_executable(${demo} "../nana-demo/${demo}.cpp")
        set_property( TARGET ${demo} PROPERTY CXX_STANDARD 14 )
        target_link_libraries(${demo} ${PROJECT_NAME})
        #if(NANA_CMAKE_AUTOMATIC_GUI_TESTING)
            #add_custom_command( TARGET  ${demo} POST_BUILD COMMAND  ${demo} )
            #add_custom_target(do_always_${demo} ALL COMMAND ${demo})
            add_test(${demo} COMMAND ${demo})
        #endif(NANA_CMAKE_AUTOMATIC_GUI_TESTING)
        install(TARGETS ${demo} RUNTIME DESTINATION  "../nana-demo/")
        message("... to build: ../nana-demo/${demo}.cpp" )
    endforeach( demo ${demos})

    set (demos     calculator  widget_show widget_show2      )

    if (NANA_CMAKE_INCLUDE_EXPERIMENTAL_DEMOS)
        list(APPEND demos  file_explorer FreeMe)
    endif (NANA_CMAKE_INCLUDE_EXPERIMENTAL_DEMOS)
    # Pending:      FreeMe (added but really completelly compiled if defined BUILD_FreeMe  )

    foreach ( demo ${demos})
        add_executable(${demo} "../nana-demo/${demo}.cpp")
        set_property( TARGET ${demo} PROPERTY CXX_STANDARD 14 )
        target_link_libraries(${demo} ${PROJECT_NAME})
        install(TARGETS ${demo} RUNTIME DESTINATION  "../nana-demo/")
        message("... to build: ../nana-demo/${demo}.cpp" )
    endforeach( demo ${demos})


    set (demos    a_group_impl  animate-bmp  audio_player audio_player_simple  background-effects categ clicked
                  decore dock  drag-button  draw example.button example_combox example_listbox example_menu
                  filebox-txt folder_tree folder_tree_nana folder_tree_std framework_design_1 framework_design_2 framework_design_3
                  group HelloWord helloword_quit inputbox label_listener lambda_event.Cpp11 listbox_inline_widget listbox_Resolver loader_1 loader_2
                  mbox menu_debug menu_popuper modal_form MontiHall place_login png screen stretch_image
                  threading thread-pool various_events window-dragger windows-subclassing
                  )
    # Pending: 
    
    foreach ( demo ${demos})
        add_executable(${demo} "../nana-demo/Examples/${demo}.cpp")
        set_property( TARGET ${demo} PROPERTY CXX_STANDARD 14 )
        target_link_libraries(${demo} ${PROJECT_NAME})
        install(TARGETS ${demo} RUNTIME DESTINATION  "../nana-demo/Examples/")
        message("... to build: ../nana-demo/Examples/${demo}.cpp" )
    endforeach( demo ${demos})


endif (NANA_CMAKE_BUILD_DEMOS)


# set compile  flags
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")
	else("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
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
message ( "DESTDIR                 = "  ${DESTDIR})
message ( "CMAKE_INSTALL_PREFIX    = "  ${CMAKE_INSTALL_PREFIX})
message ( "NANA_LINKS              = "  ${NANA_LINKS})
message ( "NANA_CMAKE_ENABLE_AUDIO = "  ${NANA_CMAKE_ENABLE_AUDIO})
message ( "NANA_CMAKE_FIND_BOOST_FILESYSTEM         = "  ${NANA_CMAKE_FIND_BOOST_FILESYSTEM})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_FORCE        = "  ${NANA_CMAKE_BOOST_FILESYSTEM_FORCE})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT = "  ${NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_LIB          = "  ${NANA_CMAKE_BOOST_FILESYSTEM_LIB})
message ( "NANA_CMAKE_AUTOMATIC_GUI_TESTING         = "  ${NANA_CMAKE_AUTOMATIC_GUI_TESTING})
message ( "NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING = "  ${NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING})


