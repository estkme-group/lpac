cmake_minimum_required (VERSION 3.8)

project (lpac
    VERSION 2.1.0
    HOMEPAGE_URL "https://github.com/estkme-group/lpac"
    DESCRIPTION "C-based eUICC LPA."
    LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LPAC_CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${LPAC_CMAKE_MODULE_PATH})

# add_compile_options(-Wall -Wextra -Wpedantic)

# Enable LTO when possible.
cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
    if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
else()
    message(INFO "IPO is not supported: ${output}")
endif()

if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

if(UNIX)
    include(GNUInstallDirs)
    if(NOT CMAKE_INSTALL_RPATH)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}/lpac")
    endif()
endif()

if(WIN32)
    add_subdirectory(dlfcn-win32)
    set(DL_LIBRARY dlfcn-win32)
else()
    set(DL_LIBRARY dl)
endif()

if(CPACK_GENERATOR)
    set(CPACK_PACKAGE_VENDOR "eSTK.me Group")
    
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "eSTK.me Group")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libcurl, libpcsclite, pcscd")

    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    
    set(CPACK_RPM_PACKAGE_LICENSE "AGPL-3.0-only AND LGPL-2.0-only")
    set(CPACK_RPM_PACKAGE_AUTOREQ "yes")
    set(CPACK_RPM_PACKAGE_REQUIRES "libcurl, libpcsclite, pcscd")

    include(CPack)
endif()

add_subdirectory(cjson)
add_subdirectory(euicc)
add_subdirectory(driver)
add_subdirectory(src)
