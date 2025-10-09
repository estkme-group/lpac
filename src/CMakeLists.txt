add_executable(lpac lpac.rc)
target_link_libraries(lpac PRIVATE euicc lpac-utils euicc-driver-loader)
target_sources(
    lpac
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
        main.c
)
target_sources(
    lpac
    PUBLIC
        FILE_SET private_headers
        TYPE HEADERS
        FILES
            main.h
            jprint.h
            applet.h
)
target_compile_options(lpac PRIVATE -Wall -Wextra -Wconversion)

include(${CMAKE_SOURCE_DIR}/cmake/git-version.cmake)
if(NOT DEFINED LPAC_VERSION)
    set(LPAC_VERSION ${CMAKE_PROJECT_VERSION})
endif()
target_compile_definitions(lpac PRIVATE LPAC_VERSION="${LPAC_VERSION}")

set_target_properties(lpac PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR}"
)

install(TARGETS lpac RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
