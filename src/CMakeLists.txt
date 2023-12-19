option(LINUX_MINGW32 "Build for windows on Linux" OFF)

if(APPLE)
    set(RPATH_BINARY_PATH "@loader_path")
else()
    set(RPATH_BINARY_PATH "$ORIGIN")
endif()

include_directories(.)
aux_source_directory(. DIR_LPAC_SRCS)
aux_source_directory(applet DIR_LPAC_SRCS)
aux_source_directory(applet/chip DIR_LPAC_SRCS)
aux_source_directory(applet/notification DIR_LPAC_SRCS)
aux_source_directory(applet/profile DIR_LPAC_SRCS)
add_executable(lpac ${DIR_LPAC_SRCS})
set_target_properties(lpac PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
    BUILD_RPATH "${RPATH_BINARY_PATH}"
)
target_link_libraries(lpac euicc)

if(LINUX_MINGW32)
    target_link_libraries(lpac dlfcn-win32)
else()
    target_link_libraries(lpac dl)
endif()
