include(CMakeDependentOption)
cmake_dependent_option(LPAC_DYNAMIC_DRIVERS "Build lpac/libeuicc driver backends as a dynamic library" OFF "LPAC_DYNAMIC_LIBEUICC" OFF)

option(LPAC_WITH_APDU_PCSC "Build APDU PCSC Backend (requires PCSC libraries)" ON)
option(LPAC_WITH_APDU_AT "Build APDU AT Backend" ON)
option(LPAC_WITH_APDU_GBINDER "Build APDU Gbinder backend for libhybris devices (requires gbinder headers)" OFF)
option(LPAC_WITH_APDU_QMI "Build QMI backend for Qualcomm devices (requires libqmi)" OFF)
option(LPAC_WITH_APDU_QMI_QRTR "Build QMI-over-QRTR backend for Qualcomm devices (requires libqrtr and libqmi headers)" OFF)
option(LPAC_WITH_APDU_MBIM "Build MBIM backend for MBIM devices (requires libmbim)" OFF)

option(LPAC_WITH_HTTP_CURL "Build HTTP Curl interface" ON)
cmake_dependent_option(LPAC_WITH_HTTP_WINHTTP "Build HTTP WinHTTP interface (Win32 only)" ON WIN32 OFF)

if(LPAC_DYNAMIC_DRIVERS)
    add_library(euicc-drivers SHARED)
    list(APPEND LIBEUICC_DRIVERS_REQUIRES "libeuicc = ${PROJECT_VERSION}")
else()
    add_library(euicc-drivers STATIC)
endif()
target_link_libraries(euicc-drivers PRIVATE euicc ${CJSON_LIBRARY} lpac-utils)
target_sources(
    euicc-drivers
    PRIVATE
        driver.c
)
target_sources(
    euicc-drivers
    PRIVATE
        FILE_SET private_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            driver.private.h
)
target_sources(
    euicc-drivers
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            driver.h
)

target_sources(euicc-drivers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apdu/stdio.c)
target_sources(euicc-drivers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/http/stdio.c)

if (LPAC_WITH_APDU_PCSC)
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_APDU_PCSC)
    add_library(driver_apdu_pcsc OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/apdu/pcsc.c)
    target_include_directories(driver_apdu_pcsc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(driver_apdu_pcsc PRIVATE euicc cjson-ext lpac-utils)
    if (WIN32)
        target_sources(driver_apdu_pcsc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apdu/pcsc_win32.c)
        target_link_libraries(driver_apdu_pcsc PRIVATE winscard)
    elseif (APPLE)
        target_link_libraries(driver_apdu_pcsc PRIVATE "-framework PCSC")
        if (LPAC_DYNAMIC_DRIVERS)
            set(LIBEUICC_DRIVERS_EXTRA_CFLAGS "-framework PCSC")
        endif ()
    else ()
        find_package(PCSCLite REQUIRED)
        target_link_libraries(driver_apdu_pcsc PRIVATE PCSCLite::PCSCLite)
        if (LPAC_DYNAMIC_DRIVERS)
            list(APPEND LIBEUICC_DRIVERS_REQUIRES "libpcsclite")
        endif ()
    endif ()
    list(APPEND LPAC_DRIVERS "driver_apdu_pcsc")
endif ()

if (LPAC_WITH_APDU_AT)
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_APDU_AT)
    add_library(driver_apdu_at OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/apdu/at_common.c)
    target_include_directories(driver_apdu_at PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(driver_apdu_at PRIVATE euicc lpac-utils)
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
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_APDU_GBINDER)
    add_library(driver_apdu_gbinder OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/apdu/gbinder_hidl.c)
    target_include_directories(driver_apdu_gbinder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GBINDER REQUIRED IMPORTED_TARGET libgbinder)
    pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(driver_apdu_gbinder PkgConfig::GBINDER PkgConfig::GLIB)
    if(LPAC_DYNAMIC_DRIVERS)
        list(APPEND LIBEUICC_DRIVERS_REQUIRES "libgbinder")
        list(APPEND LIBEUICC_DRIVERS_REQUIRES "glib-2.0")
    endif()
    list(APPEND LPAC_DRIVERS "driver_apdu_gbinder")
endif()

if (LPAC_WITH_APDU_QMI OR LPAC_WITH_APDU_QMI_QRTR)
    add_library(qmi_common OBJECT
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi_helpers.c
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi_common.c)
    target_include_directories(qmi_common PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(qmi_common cjson euicc lpac-utils)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(QMI_GLIB REQUIRED IMPORTED_TARGET qmi-glib>=1.36.0)
    target_link_libraries(qmi_common PkgConfig::QMI_GLIB)

    if (LPAC_DYNAMIC_DRIVERS)
        list(APPEND LIBEUICC_DRIVERS_REQUIRES "qmi-glib")
    endif ()
endif ()

if (LPAC_WITH_APDU_QMI)
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_APDU_QMI)
    add_library(driver_apdu_qmi OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi.c)
    target_include_directories(driver_apdu_qmi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(driver_apdu_qmi PRIVATE qmi_common)
    set_target_properties(driver_apdu_qmi PROPERTIES PREFIX "")
    target_compile_definitions(qmi_common PRIVATE LPAC_WITH_DRIVER_APDU_QMI)
    target_compile_definitions(driver_apdu_qmi PRIVATE LPAC_WITH_DRIVER_APDU_QMI)
    list(APPEND LPAC_DRIVERS "driver_apdu_qmi")
endif ()

if (LPAC_WITH_APDU_QMI_QRTR)
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_APDU_QMI_QRTR)
    add_library(driver_apdu_qmi_qrtr OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/apdu/qmi_qrtr.c)
    target_include_directories(driver_apdu_qmi_qrtr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(driver_apdu_qmi_qrtr PRIVATE qmi_common)
    set_target_properties(driver_apdu_qmi_qrtr PROPERTIES PREFIX "")
    target_compile_definitions(qmi_common PRIVATE LPAC_WITH_DRIVER_APDU_QMI_QRTR)
    target_compile_definitions(driver_apdu_qmi_qrtr PRIVATE LPAC_WITH_DRIVER_APDU_QMI_QRTR)

    if (LPAC_DYNAMIC_DRIVERS)
        list(APPEND LIBEUICC_DRIVERS_REQUIRES "qrtr-glib")
    endif ()
    list(APPEND LPAC_DRIVERS "driver_apdu_qmi_qrtr")
endif ()

if (LPAC_WITH_APDU_MBIM)
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_APDU_MBIM)
    add_library(driver_apdu_mbim OBJECT
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/mbim.c
            ${CMAKE_CURRENT_SOURCE_DIR}/apdu/mbim_helpers.c)
    target_include_directories(driver_apdu_mbim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(driver_apdu_mbim euicc lpac-utils)
    set_target_properties(driver_apdu_mbim PROPERTIES PREFIX "")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MBIM_GLIB REQUIRED IMPORTED_TARGET mbim-glib>=1.26.0)
    target_link_libraries(driver_apdu_mbim PkgConfig::MBIM_GLIB PkgConfig::MBIM_GLIB)

    if (LPAC_DYNAMIC_DRIVERS)
        list(APPEND LIBEUICC_DRIVERS_REQUIRES "mbim-glib")
    endif ()
    list(APPEND LPAC_DRIVERS "driver_apdu_mbim")
endif ()

if(LPAC_WITH_HTTP_CURL)
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_HTTP_CURL)
    add_library(driver_http_curl OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/http/curl.c)
    target_include_directories(driver_http_curl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(driver_http_curl PRIVATE euicc lpac-utils)
    if(WIN32)
        target_link_libraries(driver_http_curl PRIVATE ${DL_LIBRARY})
    else()
        find_package(CURL REQUIRED)
        target_link_libraries(driver_http_curl PRIVATE curl)
        if(LPAC_DYNAMIC_DRIVERS)
            list(APPEND LIBEUICC_DRIVERS_REQUIRES "libcurl")
        endif()
    endif()
    list(APPEND LPAC_DRIVERS "driver_http_curl")
endif()

if (LPAC_WITH_HTTP_WINHTTP)
    target_compile_definitions(euicc-drivers PRIVATE LPAC_WITH_HTTP_WINHTTP)
    add_library(driver_http_winhttp OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/http/winhttp.c)
    target_include_directories(driver_http_winhttp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(driver_http_winhttp PRIVATE winhttp euicc lpac-utils)
    set_target_properties(driver_http_winhttp PROPERTIES PREFIX "")
    list(APPEND LPAC_DRIVERS "driver_http_winhttp")
endif ()

target_link_libraries(euicc-drivers PUBLIC ${LPAC_DRIVERS})

if(LPAC_DYNAMIC_DRIVERS)
    # Install a pkg-config file (mainly for Linux; macOS is untested; Win32 is not supported)
    if(UNIX)
        list(JOIN LIBEUICC_DRIVERS_REQUIRES ", " LIBEUICC_DRIVERS_REQUIRES)
        configure_file(libeuicc-drivers.pc.in libeuicc-drivers.pc @ONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libeuicc-drivers.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    endif()
    set_target_properties(euicc-drivers PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS euicc-drivers   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                                    FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
