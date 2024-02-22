cmake_minimum_required (VERSION 3.8)
project (lpac
    VERSION 1.0.0
    HOMEPAGE_URL "https://github.com/estkme-group/lpac"
    DESCRIPTION "C-based eUICC LPA."
    LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# add_compile_options(-Wall -Wextra -Wpedantic)

if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

if(CYGWIN)
    add_definitions(-DHAVE_TIMEGM=1)
endif()

if(UNIX)
    include(GNUInstallDirs)
    if(NOT CMAKE_INSTALL_RPATH)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}/lpac")
    endif()
endif()

if(CPACK_GENERATOR)
    set(CPACK_PACKAGE_VENDOR "eSTK.me Group")
    
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "eSTK.me Group")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libcurl, libpcsclite, pcscd")
    
    set(CPACK_RPM_PACKAGE_LICENSE "AGPL-3.0-only AND LGPL-2.0-only AND MIT")
    set(CPACK_RPM_PACKAGE_AUTOREQ "yes")
    set(CPACK_RPM_PACKAGE_REQUIRES "libcurl, libpcsclite, pcscd")

    include(CPack)
endif()

if(MINGW)
    add_subdirectory(dlfcn-win32)
    set(DL_LIBRARY dlfcn-win32)
else()
    set(DL_LIBRARY dl)
endif()
add_subdirectory(cjson)
add_subdirectory(euicc)
add_subdirectory(interface)
add_subdirectory(src)
