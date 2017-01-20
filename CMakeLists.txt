# CMake configuration for Nana
# Author: Andrew Kornilov(https://github.com/ierofant)
# Contributors:
#   Jinhao	
#   Robert Hauck - Enable support for PNG/Freetype
#   Qiangqiang Wu - Add biicode support
#   Ariel Vina-Rodriguez (qPCR4vir) 
#
# Nana uses some build systems: MS-VS solution, MAKE, bakefile, codeblock, etc. manually optimized.
# In the future CMake could be the prefered, and maybe will be used to generate the others and the central nana repo
# will distribute all of them. But by now CMake is just one of them and all the other distributed build system
# files/projects are manually write. This current CMakeList.txt reflect this fact and that is why we don't
# generate here configurated *.h files or explicitly enumerate the sources files: anyway this CM-list
# will be "touched" to force a re-run of cmake.

#https://cmake.org/cmake-tutorial/
#https://cmake.org/cmake/help/v3.3/module/CMakeDependentOption.html?highlight=cmakedependentoption
# use CACHE FORCE  or set(ENABLE_MINGW_STD_THREADS_WITH_MEGANZ ON) or delete CMakecache.txt or the entirely build dir
# if your changes don't execute
#  It seems that project() defines essential system variables like CMAKE_FIND_LIBRARY_PREFIXES.
#  https://bbs.archlinux.org/viewtopic.php?id=84967

project(nana)
cmake_minimum_required(VERSION 2.8)

option(NANA_CMAKE_INSTALL_INCLUDES "Install nana includes when compile the library" ON)
option(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ "replaced boost.thread with meganz's mingw-std-threads." OFF)
option(NANA_CMAKE_ENABLE_PNG "Enable the use of PNG" OFF)
option(NANA_CMAKE_LIBPNG_FROM_OS "Use libpng from operating system." ON)
option(NANA_CMAKE_ENABLE_JPEG "Enable the use of JPEG" OFF)
option(NANA_CMAKE_LIBJPEG_FROM_OS "Use libjpeg from operating system." ON)
option(NANA_CMAKE_ENABLE_AUDIO "Enable class audio::play for PCM playback." OFF)
option(NANA_CMAKE_VERBOSE_PREPROCESSOR "Show annoying debug messages during compilation." ON)
option(NANA_CMAKE_STOP_VERBOSE_PREPROCESSOR "Stop compilation after showing the annoying debug messages." OFF)
option(NANA_CMAKE_AUTOMATIC_GUI_TESTING "Activate automatic GUI testing?" OFF)

# The ISO C++ File System Technical Specification (ISO-TS, or STD) is optional.
#              http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf
# This is not a workaround, but an user option.
# The library maybe available in the std library in use or from Boost (almost compatible)
#              http://www.boost.org/doc/libs/1_60_0/libs/filesystem/doc/index.htm
# or you can choose to use the (partial, but functional) implementation provided by nana.
# If you include the file <nana/filesystem/filesystem.hpp> or <nana/filesystem/filesystem_ext.hpp>
# the selected option will be set by nana into std::experimental::filesystem
# By default Nana will try to use the STD. If STD is not available and NANA_CMAKE_FIND_BOOST_FILESYSTEM
# is set to ON nana will try to use boost if available. Nana own implementation will be use if none of
# the previus were selected or available.
# You can change that default if you change one of the following 
# (please don't define more than one of the _XX_FORCE options):
option(NANA_CMAKE_FIND_BOOST_FILESYSTEM "Search: Is Boost filesystem available?" OFF)
option(NANA_CMAKE_NANA_FILESYSTEM_FORCE "Force nana filesystem over ISO and boost?" OFF)
option(NANA_CMAKE_STD_FILESYSTEM_FORCE "Use of STD filesystem?(a compilation error will ocurre if not available)" OFF)
option(NANA_CMAKE_BOOST_FILESYSTEM_FORCE "Force use of Boost filesystem if available (over STD)?" OFF)

########### Compatibility with CMake 3.1
if(POLICY CMP0054)
  # http://www.cmake.org/cmake/help/v3.1/policy/CMP0054.html
  cmake_policy(SET CMP0054 OLD)
endif()
if(POLICY CMP0004)    # ignore leading space
  # http://www.cmake.org/cmake/help/v3.0/policy/CMP0004.html
  cmake_policy(SET CMP0004 OLD)
endif()

########### OS

if(WIN32)
    add_definitions(-DWIN32)
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
            add_definitions(-DSTD_THREAD_NOT_SUPPORTED)
            add_definitions(-DNANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
        endif(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    add_definitions(-DAPPLE)
    include_directories(/opt/X11/include/)
    set(NANA_LINKS "${NANA_LINKS} -L/opt/X11/lib/ -liconv")
    set(ENABLE_AUDIO OFF)
elseif(UNIX)
    add_definitions(-Dlinux)
	message("added -D linux")
endif(APPLE)

if(UNIX)
    set(NANA_LINKS "${NANA_LINKS} -lX11")
    find_package(Freetype)
    if (FREETYPE_FOUND)
        include_directories( ${FREETYPE_INCLUDE_DIRS})
        set(NANA_LINKS "${NANA_LINKS} -lXft")  
    endif(FREETYPE_FOUND)
endif(UNIX)


########### Compilers
#
# Using gcc: gcc 4.8 don't support C++14 and make_unique. You may want to update at least to 4.9.
# In Windows, the gcc which come with CLion was 4.8 from MinGW. You may want to install MinGW-w64 from the
# TDM-GCC Compiler Suite for Windows which will update you to gcc 5.1.
# gcc 5.3 and 5.4 include filesytem, but you need to add the link flag: -lstdc++fs
#
# see at end of:  https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dynamic_or_shared.html
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")    # Clang || GNU

    if  ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall -g")       # Clang
	
    else ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14  -Wall -g")       # GNU 
          
    endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

endif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

# enable static linkage     # GNU || CLang not MinGW
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") #  AND NOT MINGW
    # set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++  -pthread")
    set(NANA_LINKS "${NANA_LINKS} -static-libgcc -static-libstdc++  -pthread")
    # message("Setting NANA_LINKS to -static-libgcc -static-libstdc++  -pthread or ${NANA_LINKS}")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3)
	                                                                     # IS_GNUCXX < 5.3
    else(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3)
       # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++fs")    # IS_GNUCXX 5.3 or more
       set(NANA_LINKS "${NANA_LINKS} -lstdc++fs") 
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3)
    
endif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") #  AND NOT MINGW


if (APPLE AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")                    # APPLE Clang
  # set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
  set(NANA_LINKS "${NANA_LINKS} -stdlib=libstdc++")
endif ()


############# Optional libraries

# Find PNG
if(NANA_CMAKE_ENABLE_PNG)
    add_definitions(-DNANA_ENABLE_PNG)
    set(NANA_LINKS "${NANA_LINKS} -lpng")
    if(NANA_CMAKE_LIBPNG_FROM_OS)
        find_package(PNG)
        if (PNG_FOUND)
            include_directories( ${PNG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBPNG_FROM_OS)
        endif(PNG_FOUND)
    endif(NANA_CMAKE_LIBPNG_FROM_OS)
endif(NANA_CMAKE_ENABLE_PNG)

# Find JPEG
if(NANA_CMAKE_ENABLE_JPEG)
    add_definitions(-DNANA_ENABLE_JPEG)
    set(NANA_LINKS "${NANA_LINKS} -ljpeg")
    if(NANA_CMAKE_LIBJPEG_FROM_OS)
        find_package(JPEG)
        if (JPEG_FOUND)
            include_directories( ${JPEG_INCLUDE_DIRS})
            add_definitions(-DUSE_LIBJPEG_FROM_OS)
        endif(JPEG_FOUND)
    endif(NANA_CMAKE_LIBJPEG_FROM_OS)
endif(NANA_CMAKE_ENABLE_JPEG)

# Find ASOUND
if(NANA_CMAKE_ENABLE_AUDIO)
    add_definitions(-DNANA_ENABLE_AUDIO)
    if(UNIX)
        find_package(ASOUND)
        if (ASOUND_FOUND)
            include_directories( ${ASOUND_INCLUDE_DIRS})
            set(NANA_LINKS "${NANA_LINKS}  -lasound")
        else(ASOUND_FOUND)
            message(FATAL_ERROR "libasound is not found")
        endif(ASOUND_FOUND)
    endif(UNIX)
endif(NANA_CMAKE_ENABLE_AUDIO)

# Find/Select filesystem
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
        set(NANA_LINKS "${NANA_LINKS} ${Boost_LIBRARIES}")    ######  FIRST !!!!!!!!!!!!!!!!!    add   is not first
    endif (Boost_FOUND)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)  # ??
endif (NANA_CMAKE_NANA_FILESYSTEM_FORCE)


######## Nana options

add_definitions(-DNANA_IGNORE_CONF)
if(NANA_CMAKE_VERBOSE_PREPROCESSOR)
    add_definitions(-DVERBOSE_PREPROCESSOR)
endif(NANA_CMAKE_VERBOSE_PREPROCESSOR)
if(NANA_CMAKE_AUTOMATIC_GUI_TESTING)
	 add_definitions(-DNANA_AUTOMATIC_GUI_TESTING)
	 enable_testing ()
endif(NANA_CMAKE_AUTOMATIC_GUI_TESTING)


#######################     Main setting of Nana sources, targets and install

set(NANA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(NANA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
#    collect all source sub-directories in a list to avoid duplication here
set(NANA_SOURCE_SUBDIRS              /.
                                     /detail
                                     /filesystem
                                     /gui
                                     /gui/detail
                                     /gui/widgets
                                     /gui/widgets/skeletons
                                     /paint
                                     /paint/detail
                                     /system
                                     /threads      )
if(NANA_CMAKE_ENABLE_AUDIO)
    list(APPEND NANA_SOURCE_SUBDIRS
                                     /audio
                                     /audio/detail  )
endif(NANA_CMAKE_ENABLE_AUDIO)
# collect all source files in the source-sub-dir
# To show .h files in Visual Studio, add them to the list of sources in add_executable / add_library
# and Use SOURCE_GROUP if all your sources are in the same directory
foreach(subdir ${NANA_SOURCE_SUBDIRS})
    aux_source_directory(${NANA_SOURCE_DIR}${subdir} sources)
        # message("Subir:  ${subdir}") # message("Files:  ${sources}")
endforeach(subdir ${NANA_SOURCE_SUBDIRS})

add_library(${PROJECT_NAME} ${sources} )
target_include_directories(${PROJECT_NAME} PUBLIC ${NANA_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${NANA_LINKS}) 

 #  Headers: use INCLUDE_DIRECTORIES
 #  Libraries: use FIND_LIBRARY and link with the result of it (try to avoid LINK_DIRECTORIES)

# Installing: the static "nana lib" will be in DESTDIR/CMAKE_INSTALL_PREFIX/lib/
# and the includes files "include/nana/" in DESTDIR/CMAKE_INSTALL_PREFIX/include/nana/
install(TARGETS ${PROJECT_NAME}   ARCHIVE DESTINATION lib
                                  LIBRARY DESTINATION lib)
message("The compiled Nana library will be installed in ${CMAKE_INSTALL_PREFIX}/lib")
# Install the include directories too.
if(NANA_CMAKE_INSTALL_INCLUDES)
	install(DIRECTORY ${NANA_INCLUDE_DIR}/nana DESTINATION include )
    message("The Nana include files will be installed in ${CMAKE_INSTALL_PREFIX}/include")
endif(NANA_CMAKE_INSTALL_INCLUDES)

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

# Just for information:
message ( "CMAKE_CXX_COMPILER_ID   = "  ${CMAKE_CXX_COMPILER_ID})
message ( "COMPILER_IS_CLANG       = "  ${COMPILER_IS_CLANG})
message ( "CMAKE_COMPILER_IS_GNUCXX= "  ${CMAKE_COMPILER_IS_GNUCXX})
message ( "CMAKE_CXX_FLAGS         = "  ${CMAKE_CXX_FLAGS})
message ( "CMAKE_EXE_LINKER_FLAGS  = "  ${CMAKE_EXE_LINKER_FLAGS})
message ( "CMAKE_STATIC_LINKER_FLAGS="  ${CMAKE_STATIC_LINKER_FLAGS})
message ( "NANA_LINKS              = "  ${NANA_LINKS})
message ( "DESTDIR                 = "  ${DESTDIR})
message ( "CMAKE_INSTALL_PREFIX    = "  ${CMAKE_INSTALL_PREFIX})
message ( "NANA_INCLUDE_DIR        = "  ${NANA_INCLUDE_DIR})
message ( "CMAKE_CURRENT_SOURCE_DIR= "  ${CMAKE_CURRENT_SOURCE_DIR})
message ( "NANA_CMAKE_ENABLE_AUDIO = "  ${NANA_CMAKE_ENABLE_AUDIO})
message ( "NANA_CMAKE_FIND_BOOST_FILESYSTEM         = "  ${NANA_CMAKE_FIND_BOOST_FILESYSTEM})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_FORCE        = "  ${NANA_CMAKE_BOOST_FILESYSTEM_FORCE})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT = "  ${NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_LIB          = "  ${NANA_CMAKE_BOOST_FILESYSTEM_LIB})
message ( "NANA_CMAKE_AUTOMATIC_GUI_TESTING         = "  ${NANA_CMAKE_AUTOMATIC_GUI_TESTING})
message ( "NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING = "  ${NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING})