option(LPAC_WITH_APDU_PCSC "Build APDU PCSC Backend (requires PCSC libraries)" ON)
option(LPAC_WITH_APDU_AT "Build APDU AT Backend" ON)
option(LPAC_WITH_APDU_GBINDER "Build APDU Gbinder backend for libhybris devices (requires gbinder headers)" OFF)
option(LPAC_WITH_APDU_QMI "Build QMI backend for Qualcomm devices (requires libqmi)" OFF)
option(LPAC_WITH_APDU_QMI_QRTR "Build QMI-over-QRTR backend for Qualcomm devices (requires libqrtr and libqmi headers)" OFF)
option(LPAC_WITH_APDU_UQMI "Build uQMI backend for Qualcomm devices (requires uqmi)" OFF)
option(LPAC_WITH_APDU_MBIM "Build MBIM backend for MBIM devices (requires libmbim)" OFF)

option(LPAC_WITH_HTTP_CURL "Build HTTP Curl interface" ON)
cmake_dependent_option(LPAC_WITH_HTTP_WINHTTP "Build HTTP WinHTTP interface (Win32 only)" ON WIN32 OFF)

add_library(euicc-drivers SHARED)
set_target_properties(euicc-drivers PROPERTIES LINKER_LANGUAGE C)
target_sources(
    euicc-drivers
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            driver.h
)

add_library(euicc-driver-loader SHARED)
target_link_libraries(euicc-driver-loader PRIVATE euicc euicc-drivers ${DL_LIBRARY} ${CJSON_LIBRARY} lpac-utils)
target_sources(
    euicc-driver-loader
    PRIVATE
        euicc-driver-loader.c
)
target_sources(
    euicc-driver-loader
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            euicc-driver-loader.h
)

# Only useful on Windows, and will lead to invalid arguments on ld.gold.
if(WIN32)
    target_link_options(euicc-driver-loader PRIVATE "LINKER:--export-all-symbols")
endif()

add_library(driver_apdu_stdio SHARED apdu/stdio.c)
target_link_libraries(driver_apdu_stdio PRIVATE euicc euicc-drivers lpac-utils)
set_target_properties(driver_apdu_stdio PROPERTIES PREFIX "")

add_library(driver_http_stdio SHARED http/stdio.c)
target_link_libraries(driver_http_stdio PRIVATE euicc euicc-drivers lpac-utils)
set_target_properties(driver_http_stdio PROPERTIES PREFIX "")

if (LPAC_WITH_APDU_PCSC)
    add_library(driver_apdu_pcsc SHARED ${CMAKE_CURRENT_SOURCE_DIR}/apdu/pcsc.c)
    target_link_libraries(driver_apdu_pcsc PRIVATE euicc euicc-drivers cjson-ext lpac-utils)
    set_target_properties(driver_apdu_pcsc PROPERTIES PREFIX "")
    if (WIN32)
        target_sources(driver_apdu_pcsc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apdu/pcsc_win32.c)
        target_link_libraries(driver_apdu_pcsc PRIVATE winscard)
    elseif (APPLE)
        target_link_libraries(driver_apdu_pcsc PRIVATE "-framework PCSC")
    else ()
        find_package(PCSCLite REQUIRED)
        target_link_libraries(driver_apdu_pcsc PRIVATE PCSCLite::PCSCLite)
    endif ()
    list(APPEND LPAC_DRIVERS "driver_apdu_pcsc")
endif ()

if (LPAC_WITH_APDU_AT)
    add_library(driver_apdu_at SHARED ${CMAKE_CURRENT_SOURCE_DIR}/apdu/at_common.c)
    target_link_libraries(driver_apdu_at PRIVATE euicc euicc-drivers lpac-utils)
    set_target_properties(driver_apdu_at PROPERTIES PREFIX "")
    if (UNIX)
        target_sources(driver_apdu_at PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apdu/at_unix.c)
    elseif (WIN32)
        target_link_libraries(driver_apdu_at PRIVATE setupapi)
        target_sources(driver_apdu_at PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apdu/at_win32.c)
    else ()
        message(FATAL_ERROR "LPAC_WITH_APDU_AT is only supported on Unix and Windows platforms.")
    endif ()
    list(APPEND LPAC_DRIVERS "driver_apdu_at")
endif ()

if(LPAC_WITH_APDU_GBINDER)
    add_library(driver_apdu_gbinder SHARED ${CMAKE_CURRENT_SOURCE_DIR}/apdu/gbinder_hidl.c)
    target_link_libraries(driver_apdu_gbinder PRIVATE euicc-drivers)
    set_target_properties(driver_apdu_gbinder PROPERTIES PREFIX "")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GBINDER REQUIRED IMPORTED_TARGET libgbinder)
    pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(driver_apdu_gbinder PRIVATE PkgConfig::GBINDER PkgConfig::GLIB)
    list(APPEND LPAC_DRIVERS "driver_apdu_gbinder")
endif()

if (LPAC_WITH_APDU_QMI OR LPAC_WITH_APDU_QMI_QRTR)
    add_library(qmi_common OBJECT
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi_helpers.c
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi_common.c)
    target_link_libraries(qmi_common PRIVATE cjson euicc euicc-drivers lpac-utils)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(QMI_GLIB REQUIRED IMPORTED_TARGET qmi-glib>=1.36.0)
    target_link_libraries(qmi_common PRIVATE PkgConfig::QMI_GLIB)

    # FIXME: Although it's not actual driver, but we need it to avoid strange undefined symbol.
    list(APPEND LPAC_DRIVERS "qmi_common")
endif ()

if (LPAC_WITH_APDU_QMI)
    add_library(driver_apdu_qmi SHARED ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi.c)
    target_link_libraries(driver_apdu_qmi PRIVATE euicc-drivers qmi_common)
    set_target_properties(driver_apdu_qmi PROPERTIES PREFIX "")
    target_compile_definitions(qmi_common PRIVATE LPAC_WITH_DRIVER_APDU_QMI)
    target_compile_definitions(driver_apdu_qmi PRIVATE LPAC_WITH_DRIVER_APDU_QMI)
    list(APPEND LPAC_DRIVERS "driver_apdu_qmi")
endif ()

if (LPAC_WITH_APDU_QMI_QRTR)
    add_library(driver_apdu_qmi_qrtr SHARED ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi_qrtr.c)
    target_link_libraries(driver_apdu_qmi_qrtr PRIVATE euicc-drivers qmi_common)
    set_target_properties(driver_apdu_qmi_qrtr PROPERTIES PREFIX "")
    target_compile_definitions(qmi_common PRIVATE LPAC_WITH_DRIVER_APDU_QMI_QRTR)
    target_compile_definitions(driver_apdu_qmi_qrtr PRIVATE LPAC_WITH_DRIVER_APDU_QMI_QRTR)
    list(APPEND LPAC_DRIVERS "driver_apdu_qmi_qrtr")
endif ()

if (LPAC_WITH_APDU_UQMI)
    add_library(driver_apdu_uqmi SHARED ${CMAKE_CURRENT_SOURCE_DIR}/apdu/uqmi.c)
    target_link_libraries(driver_apdu_uqmi PRIVATE euicc euicc-drivers lpac-utils)
    target_compile_options(driver_apdu_uqmi PRIVATE -Wvla -Wconversion)
    set_target_properties(driver_apdu_uqmi PROPERTIES PREFIX "")

    include(CheckSymbolExists)
    check_symbol_exists(pidfd_spawn "spawn.h" HAVE_PIDFD_SPAWN)
    check_symbol_exists(P_PIDFD "linux/wait.h" HAVE_WAITID_PIDFD)
    if (HAVE_PIDFD_SPAWN AND HAVE_WAITID_PIDFD)
        target_compile_definitions(driver_apdu_uqmi PRIVATE HAVE_PIDFD)
    endif ()

    list(APPEND LPAC_DRIVERS "driver_apdu_uqmi")
endif ()

if (LPAC_WITH_APDU_MBIM)
    add_library(driver_apdu_mbim SHARED
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/mbim.c
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/mbim_helpers.c)
    target_link_libraries(driver_apdu_mbim PRIVATE euicc euicc-drivers lpac-utils)
    set_target_properties(driver_apdu_mbim PROPERTIES PREFIX "")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MBIM_GLIB REQUIRED IMPORTED_TARGET mbim-glib>=1.26.0)
    target_link_libraries(driver_apdu_mbim PRIVATE PkgConfig::MBIM_GLIB PkgConfig::MBIM_GLIB)

    list(APPEND LPAC_DRIVERS "driver_apdu_mbim")
endif ()

if(LPAC_WITH_HTTP_CURL)
    add_library(driver_http_curl SHARED ${CMAKE_CURRENT_SOURCE_DIR}/http/curl.c)
    target_link_libraries(driver_http_curl PRIVATE euicc euicc-drivers lpac-utils)
    set_target_properties(driver_http_curl PROPERTIES PREFIX "")
    if(WIN32)
        target_link_libraries(driver_http_curl PRIVATE ${DL_LIBRARY})
    else()
        find_package(CURL REQUIRED)
        target_link_libraries(driver_http_curl PRIVATE curl)
    endif()
    list(APPEND LPAC_DRIVERS "driver_http_curl")
endif()

if (LPAC_WITH_HTTP_WINHTTP)
    add_library(driver_http_winhttp SHARED ${CMAKE_CURRENT_SOURCE_DIR}/http/winhttp.c)
    target_link_libraries(driver_http_winhttp PRIVATE winhttp euicc euicc-drivers lpac-utils)
    set_target_properties(driver_http_winhttp PROPERTIES PREFIX "")
    list(APPEND LPAC_DRIVERS "driver_http_winhttp")
endif ()

foreach(driver ${LPAC_DRIVERS})
    install(TARGETS ${driver} LIBRARY DESTINATION ${DRIVER_INSTALL_PATH})
endforeach()


# Install a pkg-config file (mainly for Linux; macOS is untested; Win32 is not supported)
if(UNIX AND NOT STANDALONE_MODE)
    list(APPEND LIBEUICC_DRIVER_LOADER_REQUIRES "libeuicc = ${PROJECT_VERSION}")
    configure_file(libeuicc-driver-loader.pc.in libeuicc-driver-loader.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libeuicc-driver-loader.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
set_target_properties(euicc-drivers PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS euicc-drivers LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
set_target_properties(euicc-driver-loader PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS euicc-driver-loader LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                                    FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
