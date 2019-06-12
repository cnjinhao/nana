option(NANA_CMAKE_INSTALL "Install nana when compile the library (to be consumed without cmake)" ON)

# Install the include directories too.
if(NANA_CMAKE_INSTALL)
    # This is a method to consume nana with a bulid system not directly supported by nana.
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

