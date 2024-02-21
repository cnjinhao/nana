option(NANA_CMAKE_ENABLE_AUDIO "Enable class audio::play for PCM playback." OFF)

# todo: decide - PUBLIC vs PRIVATE
if(NANA_CMAKE_ENABLE_AUDIO)
    target_compile_definitions(nana PUBLIC NANA_ENABLE_AUDIO)
    if(UNIX)
        find_package(ASOUND) # ? https://github.com/hintjens/demidi/blob/master/Findasound.cmake
        if(ASOUND_FOUND)
            target_include_directories(nana PUBLIC ${ASOUND_INCLUDE_DIRS})
            target_link_libraries(nana PUBLIC  ${ASOUND_LIBRARIES})
        else()
            message(FATAL_ERROR "libasound is not found")
        endif()
    endif()
endif()