option(NANA_CMAKE_INSTALL "Install nana after compiling the library (to be consumed WITHOUT cmake!!)" OFF)

# Install the include directories too.
if(NANA_CMAKE_INSTALL)
    # This is a method to consume nana with a bulid system not directly supported by nana.
    # Is your responsability to ensure all compiler options are compatible with the compilation
    # of the project linking to the nana lib here generated
    target_sources(nana PRIVATE ${HEADERS})
    message("The compiled Nana library will be installed in ${CMAKE_INSTALL_PREFIX}/lib")
    message("WARNING !!! You are using the 'installed' nana! Not recommended! ")
    message("If this was not your intention, please tern OFF option NANA_CMAKE_INSTALL ")
    message("for example by adding: -DNANA_CMAKE_INSTALL=OFF to your call to cmake. ")

    # Actually in DESTDIR/CMAKE_INSTALL_PREFIX/lib but in windows there is no DESTDIR/ part.
    install(TARGETS nana
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
    install(DIRECTORY ${NANA_INCLUDE_DIR}/nana DESTINATION include) # in ${CMAKE_INSTALL_PREFIX}/include/nana
    message("The Nana include files will be installed in ${CMAKE_INSTALL_PREFIX}/include")
    target_include_directories(nana PUBLIC $<BUILD_INTERFACE:${NANA_INCLUDE_DIR}>
                                           $<INSTALL_INTERFACE:include>  )
else()
    # this is the prefered method to consume nana with cmake
    message("You are using nana directly from original sources. (Recommended!) "
            "If this was not your intention, and what you want is to install precomplied nana first, then "
            "please tern ON option NANA_CMAKE_INSTALL ")
    target_sources(nana PUBLIC ${HEADERS})
    target_include_directories(nana PUBLIC ${NANA_INCLUDE_DIR})
endif()

