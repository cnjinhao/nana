# The ISO C++ File System Technical Specification (ISO-TS, or STD) was optional.
#              http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf
# It is part of c++17.
# The library may be not available or working correctly in the std library you use.
# As a workaround we will use Nana own partial, but functional implementation.
# You may opt to try to "implement" the std filesystem it (ab)using Boost (almost compatible)
#              http://www.boost.org/doc/libs/1_60_0/libs/filesystem/doc/index.htm
# By ncluding the file <nana/filesystem/filesystem.hpp> or <nana/filesystem/filesystem_ext.hpp>
# the selected option is inlined by nana into std::filesystem.
# By default Nana will try to use the STD. If STD is not available Nana own implementation will be used.
# You can change that default if you change one of the following
# (please don't define more than one of the _XX_FORCE options):
option(NANA_CMAKE_NANA_FILESYSTEM_FORCE "Force nana filesystem over ISO and boost?" OFF)
option(NANA_CMAKE_STD_FILESYSTEM_FORCE "Use of STD filesystem?(a compilation error will ocurre if not available)" OFF)
option(NANA_CMAKE_BOOST_FILESYSTEM_FORCE "Force use of Boost filesystem if available (over STD)?" OFF)
set (TEST_FS_LIB OFF)


if(NANA_CMAKE_NANA_FILESYSTEM_FORCE)
    if(NANA_CMAKE_STD_FILESYSTEM_FORCE)
        message (FATAL_ERROR "Defined NANA_CMAKE_NANA_FILESYSTEM_FORCE and NANA_CMAKE_STD_FILESYSTEM_FORCE")
    endif()
    if(NANA_CMAKE_BOOST_FILESYSTEM_FORCE)
        message (FATAL_ERROR "Defined NANA_CMAKE_NANA_FILESYSTEM_FORCE and NANA_CMAKE_BOOST_FILESYSTEM_FORCE")
    endif()

    target_compile_definitions(nana PUBLIC NANA_FILESYSTEM_FORCE)

elseif(NANA_CMAKE_BOOST_FILESYSTEM_FORCE)
    if(NANA_CMAKE_STD_FILESYSTEM_FORCE)
        message (FATAL_ERROR "Defined NANA_CMAKE_BOOST_FILESYSTEM_FORCE and NANA_CMAKE_STD_FILESYSTEM_FORCE")
    endif()

    target_compile_definitions(nana PUBLIC BOOST_FILESYSTEM_FORCE)
    # https://cmake.org/cmake/help/git-master/module/FindBoost.html
    # Implicit dependencies such as Boost::filesystem requiring Boost::system will be automatically detected and satisfied,
    # even if system is not specified when using find_package and if Boost::system is not added to target_link_libraries.
    # If using Boost::thread, then Thread::Thread will also be added automatically.
    find_package(Boost REQUIRED COMPONENTS filesystem)
    if(Boost_FOUND)
        target_compile_definitions(nana PUBLIC BOOST_FILESYSTEM_AVAILABLE)
            # SYSTEM - ignore warnings from here
        target_include_directories(nana SYSTEM PUBLIC "${Boost_INCLUDE_DIR}")    # ?? SYSTEM
        target_link_libraries     (nana PUBLIC ${Boost_LIBRARIES})
        # target_include_directories  (nana SYSTEM PUBLIC Boost::Boost)
        # message("boost found true")
    endif()
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)

else()

    if(NANA_CMAKE_STD_FILESYSTEM_FORCE)
        target_compile_definitions(nana PUBLIC STD_FILESYSTEM_FORCE)
    endif()

    check_include_file_cxx (filesystem              NANA_HAVE_FILESYSTEM)
    if (NANA_HAVE_FILESYSTEM)
        message (STATUS "C++ Filesystem header:      <filesystem>")
        set (TEST_FS_LIB ON)
        set (CXXSTD_FS_TEST_SOURCE
           "#include <filesystem>
            int main()
            {
                std::filesystem::path p{\"\tmp/\"};
                throw std::filesystem::filesystem_error(\"Empty file name!\", std::make_error_code(std::errc::invalid_argument));
            }")
    else()
        check_include_file_cxx (experimental/filesystem NANA_HAVE_EXP_FILESYSTEM)
        if (NANA_HAVE_EXP_FILESYSTEM)
            message (STATUS "C++ Filesystem header:      <experimental/filesystem>")
            set (TEST_FS_LIB ON)
            set (CXXSTD_FS_TEST_SOURCE
               "#include <experimental/filesystem>
                int main()
                {
                    std::experimental::filesystem::path p{\"/tmp/\"};
                    throw std::experimental::filesystem::filesystem_error(\"Empty file name!\", std::make_error_code(std::errc::invalid_argument));
                }")
        else ()
            message (WARNING "No std::filesystem include file found: nana::filesystem will be used.
                              Set NANA_CMAKE_NANA_FILESYSTEM_FORCE to ON to avoid this warning.")
            target_compile_definitions(nana PUBLIC STD_FILESYSTEM_NOT_SUPPORTED)
            set (TEST_FS_LIB OFF)
        endif ()
    endif ()

    if (TEST_FS_LIB)
        include (FindPackageMessage)
        include (CheckIncludeFileCXX)
        include (CheckCXXSourceCompiles)
        # CMAKE_REQUIRED_FLAGS = string of compile command line flags
        # CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
        # CMAKE_REQUIRED_INCLUDES = list of include directories
        set (CMAKE_REQUIRED_INCLUDES    ${CMAKE_INCLUDE_PATH})
        set (CMAKE_REQUIRED_FLAGS       ${CMAKE_CXX_FLAGS})
        set (CMAKE_REQUIRED_FLAGS_ORIGINAL ${CMAKE_REQUIRED_FLAGS})
        set (CMAKE_REQUIRED_LIBRARIES_ORIGINAL ${CMAKE_REQUIRED_LIBRARIES})

        # c++: builtin
        set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_ORIGINAL}")
        set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIGINAL})
        check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXXBuiltIn_FS_BuiltIn)
        if (CXXBuiltIn_FS_BuiltIn)
            message (STATUS "C++ Filesystem library:     builtin")
        else()
            set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} stdc++fs")
            check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXXBuiltIn_FS_stdcppfs)
            if (CXXBuiltIn_FS_stdcppfs)
                message (STATUS "C++ Filesystem library:     stdc++fs")
                target_link_libraries     (nana PUBLIC stdc++fs)
            else()
                set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} c++fs")
                check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXXBuiltIn_FS_cppfs)
                if (CXXBuiltIn_FS_cppfs)
                    message (STATUS "C++ Filesystem library:     c++fs")
                    target_link_libraries     (nana PUBLIC c++fs)
                else()

        # c++:   -std=c++17
        set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_ORIGINAL} -std=c++17")
        set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIGINAL})
        check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std__cpp17_FS_BuiltIn)
        if (CXX_std__cpp17_FS_BuiltIn)
            message (STATUS "C++: -std=c++17; Filesystem library: builtin")
        else()
            set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} stdc++fs")
            check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std__cpp17_FS_stdcppfs)
            if (CXX_std__cpp17_FS_stdcppfs)
                message (STATUS "C++: -std=c++17; Filesystem library:   stdc++fs")
                target_link_libraries     (nana PUBLIC stdc++fs)
            else()
                set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL}  c++fs")
                check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std__cpp17_FS_cppfs)
                if (CXX_std__cpp17_FS_cppfs)
                    message (STATUS "C++: -std=c++17; Filesystem library:    c++fs")
                    target_link_libraries     (nana PUBLIC  c++fs)
                else()

       # c++:   /std:c++17
       set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_ORIGINAL} /std:c++17")
       set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIGINAL})
       check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std_cpp17_FS_BuiltIn)
       if (CXX_std_cpp17_FS_BuiltIn)
           message (STATUS "C++: /std:c++17; Filesystem library: builtin")
       else()
           set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} stdc++fs")
           check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std_cpp17_FS_stdcppfs)
           if (CXX_std_cpp17_FS_stdcppfs)
               message (STATUS "C++: /std:c++17; Filesystem library:   stdc++fs")
               target_link_libraries     (nana PUBLIC stdc++fs)
           else()
               set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL}  c++fs")
               check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std_cpp17_FS_cppfs)
               if (CXX_std_cpp17_FS_cppfs)
                   message (STATUS "C++: /std:c++17; Filesystem library:    c++fs")
                   target_link_libraries     (nana PUBLIC  c++fs)

       else ()
            message (WARNING "No std::filesystem library found: nana::filesystem will be used.
                              Set NANA_CMAKE_NANA_FILESYSTEM_FORCE to ON to avoid this warning.")
            target_compile_definitions(nana PUBLIC STD_FILESYSTEM_NOT_SUPPORTED)
            set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIGINAL})
            set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_ORIGINAL}")
        endif ()
        endif ()
        endif ()
        endif ()
        endif ()
        endif ()
        endif ()
        endif ()
        endif ()

    endif (TEST_FS_LIB)
endif ()



