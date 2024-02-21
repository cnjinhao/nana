option(NANA_CMAKE_VERBOSE_PREPROCESSOR "Show annoying debug messages during compilation." OFF)
option(NANA_CMAKE_STOP_VERBOSE_PREPROCESSOR "Stop compilation after showing the annoying debug messages." OFF)


if (NANA_CMAKE_VERBOSE_PREPROCESSOR)

    target_compile_definitions(nana PRIVATE VERBOSE_PREPROCESSOR)

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

    cmake_print_variables(CMAKE_BUILD_TYPE)
    cmake_print_variables(CMAKE_CONFIGURATION_TYPES)
    cmake_print_variables(CMAKE_CXX_FLAGS_RELEASE)

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
    message ( "CMAKE_MAKE_PROGRAM      = "  ${CMAKE_MAKE_PROGRAM})
    message ( "CMAKE_CXX_COMPILER_VERSION = " ${CMAKE_CXX_COMPILER_VERSION})

    message ( "NANA_CMAKE_NANA_FILESYSTEM_FORCE         = "  ${NANA_CMAKE_NANA_FILESYSTEM_FORCE})
    message ( "NANA_CMAKE_FIND_BOOST_FILESYSTEM         = "  ${NANA_CMAKE_FIND_BOOST_FILESYSTEM})
    message ( "NANA_CMAKE_BOOST_FILESYSTEM_FORCE        = "  ${NANA_CMAKE_BOOST_FILESYSTEM_FORCE})
    message ( "NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT = "  ${NANA_CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT})
    message ( "NANA_CMAKE_BOOST_FILESYSTEM_LIB          = "  ${NANA_CMAKE_BOOST_FILESYSTEM_LIB})
    message ( "NANA_CMAKE_AUTOMATIC_GUI_TESTING         = "  ${NANA_CMAKE_AUTOMATIC_GUI_TESTING})
    message ( "NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING = "  ${NANA_CMAKE_ADD_DEF_AUTOMATIC_GUI_TESTING})

endif(NANA_CMAKE_VERBOSE_PREPROCESSOR)