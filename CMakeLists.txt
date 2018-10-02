# CMake configuration for Nana
# Contributors:
#   Andrew Kornilov (ierofant) - original version
#   Jinhao
#   Ariel Vina-Rodriguez (qPCR4vir)
#   (King_DuckZ)
#   Robert Hauck - Enable support for PNG/Freetype
#   Pavel O. - fix compilation with boost::filesystem (#281)
#   Frostbane - Add option for compiling a shared library (#263,#265)
#   Qiangqiang Wu - Add biicode support: todo migrate to https://conan.io/
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
# cmake 3.12 have more better modern c++ support

cmake_minimum_required(VERSION 3.12  FATAL_ERROR)
project(nana VERSION      1.6.2
        DESCRIPTION "C++ GUI library"
        HOMEPAGE_URL http://nanapro.org
        LANGUAGES CXX         )

#######################     Main setting of Nana targets, sources and installs    #####################

add_library(nana)
target_compile_features(nana PUBLIC cxx_std_17)

#  need after cxx_std_14 or cxx_std_17 ??
target_compile_features(nana
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

###    collect all source sub-directories in a list to avoid duplication    ###

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

###    collect all headers sub-directories in a list to avoid duplication   ###
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

###  Some nana compilation options   ###
option(NANA_CMAKE_VERBOSE_PREPROCESSOR "Show annoying debug messages during compilation." OFF)
option(NANA_CMAKE_STOP_VERBOSE_PREPROCESSOR "Stop compilation after showing the annoying debug messages." OFF)
option(NANA_CMAKE_AUTOMATIC_GUI_TESTING "Activate automatic GUI testing?" OFF)
option(NANA_CMAKE_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ "replaced boost.thread with meganz's mingw-std-threads." OFF) # deprecate?

######## Nana options

target_compile_definitions(nana PRIVATE NANA_IGNORE_CONF)    # really ?
if(NANA_CMAKE_VERBOSE_PREPROCESSOR)
    target_compile_definitions(nana PRIVATE VERBOSE_PREPROCESSOR)
endif()
if(NANA_CMAKE_AUTOMATIC_GUI_TESTING)
    target_compile_definitions(nana PUBLIC NANA_AUTOMATIC_GUI_TESTING)
    # todo: enable_testing()       #  ??
endif()

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/build/cmake/Modules)
include(build/cmake/install_nana.cmake)   # includes and libs, or just expose the nana target
include(build/cmake/OS.cmake)             # windows, unix, linux, apple, ...
include(build/cmake/shared_libs.cmake)    # static and shared
include(build/cmake/compilers.cmake)      # VC, gcc, clang

############# Optional libraries    #####################
include(build/cmake/enable_png.cmake)
include(build/cmake/enable_jpeg.cmake)
include(build/cmake/enable_audio.cmake)
include(build/cmake/select_filesystem.cmake)


###  Just for information:    ########################################
include(CMakePrintHelpers)
# see: https://cmake.org/cmake/help/v3.12/manual/cmake-properties.7.html#properties-on-targets
cmake_print_properties(TARGETS nana  PROPERTIES
        COMPILE_DEFINITIONS   COMPILE_OPTIONS    COMPILE_FLAGS  LINK_LIBRARIES
        INCLUDE_DIRECTORIES   INSTALL_NAME_DIR   LINK_FLAGS     VERSION
        )

        #message ("")
# cmake_print_variables(SOURCES)
cmake_print_variables(HEADERS)
cmake_print_variables(PUBLIC_HEADERS)
cmake_print_variables(NANA_CMAKE_INSTALL)

cmake_print_variables(Boost_INCLUDE_DIR)
cmake_print_variables(Boost_LIBRARIES)
cmake_print_variables(Boost::filesystem)

cmake_print_variables(PNG_INCLUDE_DIRS)
cmake_print_variables(PNG_LIBRARIES)
cmake_print_variables(PNG::PNG)


message ( "CMAKE_CXX_COMPILER_ID     = "  ${CMAKE_CXX_COMPILER_ID})
message ( "COMPILER_IS_CLANG         = "  ${COMPILER_IS_CLANG})
message ( "CMAKE_COMPILER_IS_GNUCXX  = "  ${CMAKE_COMPILER_IS_GNUCXX})
message ( "CMAKE_CXX_FLAGS           = "  ${CMAKE_CXX_FLAGS})
message ( "CMAKE_EXE_LINKER_FLAGS    = "  ${CMAKE_EXE_LINKER_FLAGS})
message ( "CMAKE_STATIC_LINKER_FLAGS = "  ${CMAKE_STATIC_LINKER_FLAGS})

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
