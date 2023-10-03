option(LINUX_MINGW32 "Build for windows on Linux" OFF)


add_library(es9pinterface_curl SHARED curl.c)
set_target_properties(es9pinterface_curl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(es9pinterface_curl PROPERTIES LINK_FLAGS "-flto -s -Wl,--build-id=none")
target_link_libraries(es9pinterface_curl curl)

