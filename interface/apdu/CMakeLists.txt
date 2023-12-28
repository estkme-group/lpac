option(LINUX_MINGW32 "Build for windows on Linux" OFF)

add_library(apduinterface_pcsc SHARED pcsc.c)
target_link_libraries(apduinterface_pcsc cjson-static)
set_target_properties(apduinterface_pcsc PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(apduinterface_pcsc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")

if(LINUX_MINGW32 OR CYGWIN)
    target_link_libraries(apduinterface_pcsc winscard)
elseif (APPLE)
    target_link_libraries(apduinterface_pcsc "-framework PCSC")
else()
    include_directories("/usr/include/PCSC")
    target_link_libraries(apduinterface_pcsc pcsclite)
endif()

add_library(apduinterface_at SHARED at.c)
set_target_properties(apduinterface_at PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(apduinterface_at PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")

add_library(apduinterface_stdio SHARED stdio.c)
target_link_libraries(apduinterface_stdio cjson-static)
set_target_properties(apduinterface_stdio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(apduinterface_stdio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
