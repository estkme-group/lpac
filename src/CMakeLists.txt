option(LINUX_MINGW32 "Build for windows on Linux" OFF)

aux_source_directory(. DIR_SRCS)
add_executable(lpac ${DIR_SRCS})
set_target_properties(lpac PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
target_link_libraries(lpac euicc)
target_link_libraries(lpac cjson-static)

if(LINUX_MINGW32)
    target_link_libraries(lpac dlfcn-win32)
else()
    target_link_libraries(lpac dl)
endif()
