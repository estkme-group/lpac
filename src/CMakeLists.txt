if(APPLE)
    set(RPATH_BINARY_PATH "@loader_path")
else()
    set(RPATH_BINARY_PATH "$ORIGIN")
endif()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/chip DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/notification DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/profile DIR_LPAC_SRCS)
add_executable(lpac ${DIR_LPAC_SRCS})
set_target_properties(lpac PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
    BUILD_RPATH "${RPATH_BINARY_PATH}"
)
target_link_libraries(lpac euicc ${DL_LIBRARY})
target_include_directories(lpac PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if(UNIX)
    install(TARGETS lpac RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
