option(LINUX_MINGW32 "Build for windows on Linux" OFF)


add_library(httpinterface_curl SHARED curl.c)
target_link_libraries(httpinterface_curl curl)
set_target_properties(httpinterface_curl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(httpinterface_curl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")


add_library(httpinterface_stdio SHARED stdio.c)
target_link_libraries(httpinterface_stdio cjson-static)
set_target_properties(httpinterface_stdio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(httpinterface_stdio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
