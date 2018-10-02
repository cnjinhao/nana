# CMake configuration for Nana
# Author: Andrew Kornilov(https://github.com/ierofant)
# Contributors:
#   Andrew Kornilov (ierofant) - original version
#   Jinhao
#   Ariel Vina-Rodriguez (qPCR4vir)
#   (King_DuckZ)
#   Robert Hauck - Enable support for PNG/Freetype
#   Pavel O. - fix compilation with boost::filesystem (#281)
#   Frostbane - Add option for compiling a shared library (#263,#265)
#   Qiangqiang Wu - Add biicode support
#
# Nana uses some build systems: MS-VS solution, MAKE, bakefile, codeblock, etc. manually optimized.
# Maybe CMake will be used in the future to generate some of them in the central nana repository.
# But by now CMake is just one option and all the other build system
# files/projects distributed are manually writen. This current CMakeList.txt reflect this fact and that is why we don't
# generate here configurated *.h files or explicitly enumerate the sources files: anyway this CM-list
# will be "touched" to force a re-run of cmake.

# https://cliutils.gitlab.io/modern-cmake/
# https://cmake.org/cmake-tutorial/
# https://cmake.org/cmake/help/v3.12/module/CMakeDependentOption.html?highlight=cmakedependentoption

cmake_minimum_required(VERSION 3.12  FATAL_ERROR)
project(nana VERSION      1.6.2
        DESCRIPTION "C++ GUI library"
        HOMEPAGE_URL http://nanapro.org
        LANGUAGES CXX         )


option(BUILD_SHARED_LIBS "Compile nana as a shared library." OFF)

#######################     Main setting of Nana targets, sources and installs

add_library(nana)
target_compile_features(nana PUBLIC cxx_std_14)

###    collect all source sub-directories in a list to avoid duplication here

# By using CMAKE_CURRENT_LIST_DIR here you can compile and consume nana by just:
# add_subdirectory(../nana ../cmake-nana-build-${CONFIG} ) or simmilar
# in your own CMakeLists.txt, and them :
# target_link_libraries(yourApp PRIVATE nana )

set(NANA_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/source)

set(NANA_SOURCE_SUBDIRS
        /.
        /detail
        /filesystem
        /gui
        /gui/detail
        /gui/widgets
        /gui/widgets/skeletons
        /paint
        /paint/detail
        /system
        /threads
        )
if(NANA_CMAKE_ENABLE_AUDIO)
    list(APPEND NANA_SOURCE_SUBDIRS
            /audio
            /audio/detail
            )
endif()

# collect all source files in the source-sub-dir
foreach(subdir ${NANA_SOURCE_SUBDIRS})
    aux_source_directory(${NANA_SOURCE_DIR}${subdir} SOURCES) # todo: use GLOB to add headers too ??
endforeach()

target_sources(nana PRIVATE ${SOURCES})


###    collect all headers sub-directories in a list to avoid duplication here ??
# To show .h files in Visual Studio, add them to the list of sources in add_executable / add_library / target_sources
# and Use SOURCE_GROUP if all your sources are in the same directory
set(NANA_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)

set(NANA_INCLUDE_SUBDIRS
        /.
        /filesystem
        /gui
        /gui/detail
        /gui/widgets
        /gui/widgets/skeletons
        /paint
        /paint/detail
        /pat
        /system
        /threads
        )
if(NANA_CMAKE_ENABLE_AUDIO)
    list(APPEND NANA_INCLUDE_SUBDIRS
            /audio
            /audio/detail
            )
endif()

foreach(subdir ${NANA_INCLUDE_SUBDIRS})
    aux_source_directory(${NANA_INCLUDE_DIR}/nana${subdir} HEADERS)  # todo: use GLOB to add headers too !!!!!!!
endforeach()


option(NANA_CMAKE_INSTALL "Install nana when compile the library" OFF)
option(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ "replaced boost.thread with meganz's mingw-std-threads." OFF)
option(NANA_CMAKE_VERBOSE_PREPROCESSOR "Show annoying debug messages during compilation." ON)
option(NANA_CMAKE_STOP_VERBOSE_PREPROCESSOR "Stop compilation after showing the annoying debug messages." OFF)
option(NANA_CMAKE_AUTOMATIC_GUI_TESTING "Activate automatic GUI testing?" OFF)


# Install the include directories too.
if(NANA_CMAKE_INSTALL)
    # this is the prefered method to consume nana directly with some specific bulid system
    # Is your responsability to ensure all compiler options are compatible with the compilation
    # of the project linking to the nana lib here generated
    target_sources(nana PRIVATE ${HEADERS})
    target_include_directories(nana PRIVATE ${NANA_INCLUDE_DIR})
    message("The compiled Nana library will be installed in ${CMAKE_INSTALL_PREFIX}/lib")
    # Actually in DESTDIR/CMAKE_INSTALL_PREFIX/lib but in windows there is no DESTDIR/ part.
    install(TARGETS nana
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
    install(DIRECTORY ${NANA_INCLUDE_DIR}/nana DESTINATION include) # in ${CMAKE_INSTALL_PREFIX}/include/nana
    message("The Nana include files will be installed in ${CMAKE_INSTALL_PREFIX}/include")
else()
    # this is the prefered method to consume nana with cmake
    target_sources(nana PUBLIC ${HEADERS})
    target_include_directories(nana PUBLIC ${NANA_INCLUDE_DIR})
endif()


if(BUILD_SHARED_LIBS)    # ??
    include(build/cmake/shared_libs.cmake)
endif()

######## Nana options

target_compile_definitions(nana PRIVATE NANA_IGNORE_CONF)    # really ?
if(NANA_CMAKE_VERBOSE_PREPROCESSOR)
    target_compile_definitions(nana PRIVATE VERBOSE_PREPROCESSOR)
endif()
if(NANA_CMAKE_AUTOMATIC_GUI_TESTING)
    target_compile_definitions(nana PUBLIC NANA_AUTOMATIC_GUI_TESTING)
    # todo: enable_testing()       #  ??
endif()

########### OS
# https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
# http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system

if(WIN32)
    target_compile_definitions(nana PUBLIC WIN32)    # todo: why not simple test for _WIN32 in code??
    set(CMAKE_DEBUG_POSTFIX "_d")
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
            target_compile_definitions(nana PUBLIC STD_THREAD_NOT_SUPPORTED)
            target_compile_definitions(nana PUBLIC NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
        endif()
    endif()
endif()

if(APPLE)
    target_compile_definitions(nana PUBLIC APPLE)      # ??? not added by compilers? use  __APPLE__  ?
    target_include_directories(nana PUBLIC /opt/X11/include/)
    target_link_libraries(nana PRIVATE iconv)
    set(ENABLE_AUDIO OFF)
elseif(UNIX)
    target_compile_definitions(nana PUBLIC linux)      # todo: eliminate. Added by compilers. Also __linux posible
endif()

if(UNIX)
    find_package(X11 REQUIRED)
    target_link_libraries(nana
        PUBLIC ${X11_LIBRARIES}
        PUBLIC ${X11_Xft_LIB}
    )
    target_include_directories(nana SYSTEM
        PUBLIC ${X11_Xft_INCLUDE_PATH}
        PUBLIC ${X11_INCLUDE_DIR}
    )
    find_package(Freetype)
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


########### Compilers
#
# Using gcc: gcc 4.8 don't support C++14 and make_unique. You may want to update at least to 4.9.
# gcc 5.3 and 5.4 include filesytem, but you need to add the link flag: -lstdc++fs
#
# In Windows, with CLion Allways check in File/Settings.../toolchains
# You could install MinGW-w64 from the TDM-GCC Compiler Suite for Windows which will update you to gcc 5.1.
# It is posible to follow https://computingabdn.com/softech/mingw-howto-install-gcc-for-windows/
# and install MinGW with gcc 7.1 with has STD_THREADS and fs, from: https://sourceforge.net/projects/mingw-w64/files/
#
# see at end of:  https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dynamic_or_shared.html
#
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

    target_compile_options(nana PRIVATE  -Wall)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        if("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
            target_compile_options(nana PUBLIC -I/usr/local/include)
        endif()
    endif()
#    set(THREADS_PREFER_PTHREAD_FLAG ON)
#    find_package(Threads REQUIRED)
#    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

    if(BUILD_SHARED_LIBS)
        target_compile_options(nana PUBLIC  -lgcc -lstdc++)
    else()

        if(MINGW)
            target_compile_options(nana PUBLIC -static)     #  -static ?? cmake knows BUILD_SHARED_LIBS
        else()
            target_compile_options(nana PUBLIC -static-libgcc -static-libstdc++)
        endif()
    endif(BUILD_SHARED_LIBS)
       # target_link_libraries(nana PRIVATE stdc++fs)    # ??

    target_compile_options(nana PUBLIC -pthread)

endif()


if (APPLE AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")                    # APPLE Clang
    target_compile_options(nana PUBLIC -stdlib=libstdc++)
endif ()


############# Optional libraries
include(build/cmake/enable_png.cmake)
include(build/cmake/enable_jpeg.cmake)
include(build/cmake/enable_audio.cmake)
include(build/cmake/select_filesystem.cmake)


if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_compile_options(nana PRIVATE -fmax-errors=3)
endif()



target_compile_features(${PROJECT_NAME}
    PUBLIC cxx_nullptr
    PUBLIC cxx_range_for
    PUBLIC cxx_lambdas
    PUBLIC cxx_decltype_auto
    PUBLIC cxx_defaulted_functions
    PUBLIC cxx_deleted_functions
    PUBLIC cxx_auto_type
    PUBLIC cxx_decltype_incomplete_return_types
    PUBLIC cxx_defaulted_move_initializers
    PUBLIC cxx_noexcept
    PUBLIC cxx_rvalue_references
)

# Just for information:
include(CMakePrintHelpers)

message ("")
# cmake_print_variables(SOURCES)
cmake_print_variables(HEADERS)
cmake_print_variables(PUBLIC_HEADERS)
cmake_print_variables(NANA_CMAKE_INSTALL)

message ( "CMAKE_CXX_COMPILER_ID     = "  ${CMAKE_CXX_COMPILER_ID})
message ( "COMPILER_IS_CLANG         = "  ${COMPILER_IS_CLANG})
message ( "CMAKE_COMPILER_IS_GNUCXX  = "  ${CMAKE_COMPILER_IS_GNUCXX})
message ( "CMAKE_CXX_FLAGS           = "  ${CMAKE_CXX_FLAGS})
message ( "CMAKE_EXE_LINKER_FLAGS    = "  ${CMAKE_EXE_LINKER_FLAGS})
message ( "CMAKE_STATIC_LINKER_FLAGS = "  ${CMAKE_STATIC_LINKER_FLAGS})


# see: https://cmake.org/cmake/help/v3.12/manual/cmake-properties.7.html#properties-on-targets
cmake_print_properties(TARGETS nana  PROPERTIES
        COMPILE_DEFINITIONS   COMPILE_OPTIONS    COMPILE_FLAGS  LINK_LIBRARIES
        INCLUDE_DIRECTORIES   INSTALL_NAME_DIR   LINK_FLAGS     VERSION
                                      )


message ( "DESTDIR                   = "  ${DESTDIR})
message ( "CMAKE_INSTALL_PREFIX      = "  ${CMAKE_INSTALL_PREFIX})
message ( "NANA_INCLUDE_DIR          = "  ${NANA_INCLUDE_DIR})
message ( "CMAKE_CURRENT_SOURCE_DIR  = "  ${CMAKE_CURRENT_SOURCE_DIR})
message ( "NANA_CMAKE_ENABLE_AUDIO   = "  ${NANA_CMAKE_ENABLE_AUDIO})
message ( "NANA_CMAKE_SHARED_LIB     = "  ${NANA_CMAKE_SHARED_LIB})
message ( "NANA_CLION              = "  ${NANA_CLION})
message ( "CMAKE_MAKE_PROGRAM      = "  ${CMAKE_MAKE_PROGRAM})
message ( "CMAKE_CXX_COMPILER_VERSION = " ${CMAKE_CXX_COMPILER_VERSION})

message ( "NANA_CMAKE_NANA_FILESYSTEM_FORCE         = "  ${NANA_CMAKE_NANA_FILESYSTEM_FORCE})
message ( "NANA_CMAKE_FIND_BOOST_FILESYSTEM         = "  ${NANA_CMAKE_FIND_BOOST_FILESYSTEM})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_FORCE        = "  ${NANA_CMAKE_BOOST_FILESYSTEM_FORCE})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT = "  ${NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT})
message ( "NANA_CMAKE_BOOST_FILESYSTEM_LIB          = "  ${NANA_CMAKE_BOOST_FILESYSTEM_LIB})
message ( "NANA_CMAKE_AUTOMATIC_GUI_TESTING         = "  ${NANA_CMAKE_AUTOMATIC_GUI_TESTING})
message ( "NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING = "  ${NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING})
