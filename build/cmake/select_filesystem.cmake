
include (CheckIncludeFileCXX)
check_include_file_cxx (filesystem              NANA_HAVE_FILESYSTEM)
if (NOT NANA_HAVE_FILESYSTEM)
    message (ERROR "No std::filesystem include file found.")
endif ()    

message (STATUS "C++ Filesystem header:      <filesystem>")

include (FindPackageMessage)

include (CheckCXXSourceCompiles)
# set flags used only for cmake testing, not for project compilation!!
    # CMAKE_REQUIRED_FLAGS = string of compile command line flags
    # CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
    # CMAKE_REQUIRED_INCLUDES = list of include directories
    set (CMAKE_REQUIRED_INCLUDES    ${CMAKE_INCLUDE_PATH})
    set (CMAKE_REQUIRED_FLAGS       ${CMAKE_CXX_FLAGS})
    set (CMAKE_REQUIRED_FLAGS_ORIGINAL ${CMAKE_REQUIRED_FLAGS})
    set (CMAKE_REQUIRED_LIBRARIES_ORIGINAL ${CMAKE_REQUIRED_LIBRARIES})
    set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_ORIGINAL}")
    set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIGINAL})


set (CXXSTD_FS_TEST_SOURCE
        "#include <filesystem>
        int main()
        {
            std::filesystem::path p{\"\tmp/\"};
            throw std::filesystem::filesystem_error(\"Empty file name!\", std::make_error_code(std::errc::invalid_argument));
        }")


# c++: builtin fs library ?
check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXXBuiltIn_FS_BuiltIn)
if (CXXBuiltIn_FS_BuiltIn)
    message (STATUS "C++ Filesystem library:     builtin")

# or we need stdc++fs library ?
else()
    set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} stdc++fs")
    check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXXBuiltIn_FS_stdcppfs)
    if (CXXBuiltIn_FS_stdcppfs)
        message (STATUS "C++ Filesystem library:     stdc++fs")
        target_link_libraries     (nana PUBLIC stdc++fs)

    else()
        # or we need c++fs library ?
        set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} c++fs")
        check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXXBuiltIn_FS_cppfs)
        if (CXXBuiltIn_FS_cppfs)
            message (STATUS "C++ Filesystem library:     c++fs")
            target_link_libraries     (nana PUBLIC c++fs)

        # or we also need some c++ flag ?
        #   -std=c++17 with builtin fs library
        else()
            set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_ORIGINAL} -std=c++17")
            set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIGINAL})
            check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std__cpp17_FS_BuiltIn)
            if (CXX_std__cpp17_FS_BuiltIn)
                message (STATUS "C++: -std=c++17; Filesystem library: builtin")

            #   -std=c++17 with stdc++fs library
            else()
                set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} stdc++fs")
                check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std__cpp17_FS_stdcppfs)
                if (CXX_std__cpp17_FS_stdcppfs)
                    message (STATUS "C++: -std=c++17; Filesystem library:   stdc++fs")
                    target_link_libraries     (nana PUBLIC stdc++fs)

                #   -std=c++17 with c++fs library
                else()
                    set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL}  c++fs")
                    check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std__cpp17_FS_cppfs)
                    if (CXX_std__cpp17_FS_cppfs)
                        message (STATUS "C++: -std=c++17; Filesystem library:    c++fs")
                        target_link_libraries     (nana PUBLIC  c++fs)
                
                    #   /std:c++17 with builtin fs library
                    else()
                        set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_ORIGINAL} /std:c++17")
                        set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_ORIGINAL})
                        check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std_cpp17_FS_BuiltIn)
                        if (CXX_std_cpp17_FS_BuiltIn)
                            message (STATUS "C++: /std:c++17; Filesystem library: builtin")
                    
                        #   /std:c++17 with stdc++fs library
                        else()
                    
                            set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL} stdc++fs")
                            check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std_cpp17_FS_stdcppfs)
                            if (CXX_std_cpp17_FS_stdcppfs)
                                message (STATUS "C++: /std:c++17; Filesystem library:   stdc++fs")
                                target_link_libraries     (nana PUBLIC stdc++fs)
                        
                            #   /std:c++17 with c++fs library
                            else()
                                set (CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES_ORIGINAL}  c++fs")
                                check_cxx_source_compiles ("${CXXSTD_FS_TEST_SOURCE}" CXX_std_cpp17_FS_cppfs)
                                if (CXX_std_cpp17_FS_cppfs)
                                    message (STATUS "C++: /std:c++17; Filesystem library:    c++fs")
                                    target_link_libraries     (nana PUBLIC  c++fs)
                                else()
                                endif ()
                            endif ()
                        endif ()
                    endif ()
                endif ()
            endif ()
    endif ()
endif ()






