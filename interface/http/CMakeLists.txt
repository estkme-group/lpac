option(LINUX_MINGW32 "Build for windows on Linux" OFF)


add_library(httpinterface_curl SHARED curl.c)
set_target_properties(httpinterface_curl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
target_link_libraries(httpinterface_curl curl)

add_library(httpinterface_stdio SHARED stdio.c)
set_target_properties(httpinterface_stdio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
target_link_libraries(httpinterface_stdio cjson-static)
