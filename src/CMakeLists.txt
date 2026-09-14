add_library(lpac-lib STATIC)
target_link_libraries(lpac-lib PRIVATE cjson-ext euicc lpac-utils euicc-driver-loader)
target_sources(
    lpac-lib
    PRIVATE
        applet/chip/defaultsmdp.c
        applet/chip/info.c
        applet/chip/purge.c
        applet/notification/list.c
        applet/notification/notification_common.c
        applet/notification/process.c
        applet/notification/remove.c
        applet/notification/replay.c
        applet/notification/dump.c
        applet/profile/delete.c
        applet/profile/disable.c
        applet/profile/discovery.c
        applet/profile/enable.c
        applet/profile/list.c
        applet/profile/nickname.c
        applet/profile/download.c
        applet/chip.c
        applet/notification.c
        applet/profile.c
        applet/version.c
        applet.c
        jprint.c
        lpac.c
)
target_sources(
    lpac-lib
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        FILES
            lpac.h
)
target_compile_options(lpac-lib PRIVATE -Wall -Wextra -Wconversion)

add_library(lpac-internal INTERFACE)
target_sources(
    lpac-internal
    INTERFACE
        FILE_SET private_headers
        TYPE HEADERS
        FILES
            main.h
            jprint.h
            applet.h
)

add_executable(lpac lpac.rc)
target_link_libraries(lpac PRIVATE lpac-lib)
target_sources(
    lpac
    PRIVATE
        main.c
)
target_compile_options(lpac PRIVATE -Wall -Wextra -Wconversion)

include(${CMAKE_SOURCE_DIR}/cmake/git-version.cmake)
if(NOT DEFINED LPAC_VERSION)
    set(LPAC_VERSION ${CMAKE_PROJECT_VERSION})
endif()
target_compile_definitions(lpac-lib PRIVATE LPAC_VERSION="${LPAC_VERSION}")

set_target_properties(lpac PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR}"
)

install(TARGETS lpac RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
