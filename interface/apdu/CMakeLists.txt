option(LPAC_APDU_INTERFACE_PCSC "Build PCSC backend (requires PCSC libraries)" ON)
option(LPAC_APDU_INTERFACE_GBINDER "Build GBinder backend for libhybris devices (requires gbinder headers)" OFF)

if(LPAC_APDU_INTERFACE_PCSC)
    add_library(apduinterface_pcsc SHARED pcsc.c)
    target_link_libraries(apduinterface_pcsc euicc cjson-static)
    set_target_properties(apduinterface_pcsc PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
    set_target_properties(apduinterface_pcsc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
    if(UNIX)
        install(TARGETS apduinterface_pcsc LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/lpac")
    endif()

    if(MINGW OR CYGWIN)
        target_link_libraries(apduinterface_pcsc winscard)
    elseif(APPLE)
        target_link_libraries(apduinterface_pcsc "-framework PCSC")
    else()
        find_package(PCSCLite)
        target_link_libraries(apduinterface_pcsc PCSCLite::PCSCLite)
    endif()
endif()

add_library(apduinterface_at SHARED at.c)
target_link_libraries(apduinterface_at euicc)
set_target_properties(apduinterface_at PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(apduinterface_at PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
if(UNIX)
    install(TARGETS apduinterface_at LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/lpac")
endif()

add_library(apduinterface_stdio SHARED stdio.c)
target_link_libraries(apduinterface_stdio euicc cjson-static)
set_target_properties(apduinterface_stdio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
set_target_properties(apduinterface_stdio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
if(UNIX)
    install(TARGETS apduinterface_stdio LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/lpac")
endif()

if(LPAC_APDU_INTERFACE_GBINDER)
    add_library(apduinterface_gbinder_hidl SHARED gbinder_hidl.c)
    target_link_libraries(apduinterface_gbinder_hidl euicc)
    set_target_properties(apduinterface_gbinder_hidl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
    set_target_properties(apduinterface_gbinder_hidl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output")
    install(TARGETS apduinterface_gbinder_hidl LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/lpac")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GBINDER REQUIRED IMPORTED_TARGET libgbinder)
    target_link_libraries(apduinterface_gbinder_hidl PkgConfig::GBINDER)
endif()

if(CYGWIN)
    if(LPAC_APDU_INTERFACE_PCSC)
        add_custom_command(TARGET apduinterface_pcsc POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rename
                ${CMAKE_BINARY_DIR}/output/cygapduinterface_pcsc.dll ${CMAKE_BINARY_DIR}/output/libapduinterface_pcsc.dll
            COMMAND ${CMAKE_COMMAND} -E echo
                "Renamed ${CMAKE_BINARY_DIR}/output/cygapduinterface_pcsc.dll to ${CMAKE_BINARY_DIR}/output/libapduinterface_pcsc.dll")
    endif()
    add_custom_command(TARGET apduinterface_at POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
            ${CMAKE_BINARY_DIR}/output/cygapduinterface_at.dll ${CMAKE_BINARY_DIR}/output/libapduinterface_at.dll
        COMMAND ${CMAKE_COMMAND} -E echo
            "Renamed ${CMAKE_BINARY_DIR}/output/cygapduinterface_at.dll to ${CMAKE_BINARY_DIR}/output/libapduinterface_at.dll")
    add_custom_command(TARGET apduinterface_stdio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
            ${CMAKE_BINARY_DIR}/output/cygapduinterface_stdio.dll ${CMAKE_BINARY_DIR}/output/libapduinterface_stdio.dll
        COMMAND ${CMAKE_COMMAND} -E echo
            "Renamed ${CMAKE_BINARY_DIR}/output/cygapduinterface_stdio.dll to ${CMAKE_BINARY_DIR}/output/libapduinterface_stdio.dll")
endif()
