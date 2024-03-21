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
target_link_libraries(lpac euicc-drivers)
target_include_directories(lpac PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

find_package(Git)
add_custom_target(version
    ${CMAKE_COMMAND}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    -D DST=${CMAKE_CURRENT_SOURCE_DIR}/version.h
    -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
    -P ${CMAKE_MODULE_PATH}/git-version.cmake
)
add_dependencies(lpac version)
set_target_properties(lpac PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
    BUILD_RPATH "${RPATH_BINARY_PATH}"
)

if(UNIX)
    install(TARGETS lpac RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
