add_library(httpinterface_curl SHARED curl.c)
target_link_libraries(httpinterface_curl euicc ${DL_LIBRARY})
set_target_properties(httpinterface_curl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(httpinterface_curl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
if(UNIX)
    install(TARGETS httpinterface_curl LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/lpac")
endif()


add_library(httpinterface_stdio SHARED stdio.c)
target_link_libraries(httpinterface_stdio euicc cjson-static)
set_target_properties(httpinterface_stdio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(httpinterface_stdio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
if(UNIX)
    install(TARGETS httpinterface_stdio LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/lpac")
endif()

if(CYGWIN)
    add_custom_command(TARGET httpinterface_curl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
            ${CMAKE_BINARY_DIR}/output/cyghttpinterface_curl.dll ${CMAKE_BINARY_DIR}/output/libhttpinterface_curl.dll
        COMMAND ${CMAKE_COMMAND} -E echo
            "Renamed ${CMAKE_BINARY_DIR}/output/cyghttpinterface_curl.dll to ${CMAKE_BINARY_DIR}/output/libhttpinterface_curl.dll")
    add_custom_command(TARGET httpinterface_stdio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
            ${CMAKE_BINARY_DIR}/output/cyghttpinterface_stdio.dll ${CMAKE_BINARY_DIR}/output/libhttpinterface_stdio.dll
        COMMAND ${CMAKE_COMMAND} -E echo
            "Renamed ${CMAKE_BINARY_DIR}/output/cyghttpinterface_stdio.dll to ${CMAKE_BINARY_DIR}/output/libhttpinterface_stdio.dll")
endif()
