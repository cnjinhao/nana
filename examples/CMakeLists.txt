macro(add_example target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} LINK_PRIVATE nana)
endmacro()

add_example(helloworld "helloworld.cpp")

add_subdirectory(label)
add_subdirectory(button)
