option(LINUX_MINGW32 "Build for windows on Linux" OFF)


add_library(httpinterface_curl SHARED curl.c)
if(LINUX_MINGW32)
    target_link_libraries(httpinterface_curl dlfcn-win32)
else()
    target_link_libraries(httpinterface_curl dl)
endif()
set_target_properties(httpinterface_curl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(httpinterface_curl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")


add_library(httpinterface_stdio SHARED stdio.c)
target_link_libraries(httpinterface_stdio cjson-static)
set_target_properties(httpinterface_stdio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(httpinterface_stdio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
