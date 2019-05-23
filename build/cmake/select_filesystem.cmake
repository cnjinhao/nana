# The ISO C++ File System Technical Specification (ISO-TS, or STD) was optional.
#              http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf
# It is part of c++17.
# The library may be not available or working correctly in the std library in use. As a workaround we may try
# to "implement" it (ab)using Boost (almost compatible)
#              http://www.boost.org/doc/libs/1_60_0/libs/filesystem/doc/index.htm
# or you can choose to use the (partial, but functional) implementation provided by nana.
# If you include the file <nana/filesystem/filesystem.hpp> or <nana/filesystem/filesystem_ext.hpp>
# the selected option will be set by nana into std::filesystem
# By default Nana will try to use the STD. If STD is not available and NANA_CMAKE_FIND_BOOST_FILESYSTEM
# is set to ON nana will try to use boost if available. Nana own implementation will be use if none of
# the previus were selected or available.
# You can change that default if you change one of the following
# (please don't define more than one of the _XX_FORCE options):
option(NANA_CMAKE_NANA_FILESYSTEM_FORCE "Force nana filesystem over ISO and boost?" OFF)
option(NANA_CMAKE_STD_FILESYSTEM_FORCE "Use of STD filesystem?(a compilation error will ocurre if not available)" OFF)
option(NANA_CMAKE_BOOST_FILESYSTEM_FORCE "Force use of Boost filesystem if available (over STD)?" OFF)
option(NANA_CMAKE_FIND_BOOST_FILESYSTEM "Search: Is Boost filesystem available?" OFF)

if(NANA_CMAKE_NANA_FILESYSTEM_FORCE)
    target_compile_definitions(nana PUBLIC NANA_FILESYSTEM_FORCE)

elseif(NANA_CMAKE_STD_FILESYSTEM_FORCE)
    target_compile_definitions(nana PUBLIC STD_FILESYSTEM_FORCE)
    target_link_libraries     (nana PUBLIC stdc++fs)

elseif(NANA_CMAKE_BOOST_FILESYSTEM_FORCE)
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
    # todo   test for std    (for now just force nana or boost if there no std)
    target_link_libraries     (nana PUBLIC stdc++fs)

    # todo if not test for boost
    # if not add nana filesystem
endif()





