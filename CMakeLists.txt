cmake_minimum_required (VERSION 3.15)

project (lpac
    VERSION 2.3.0
    HOMEPAGE_URL "https://github.com/estkme-group/lpac"
    DESCRIPTION "C-based eUICC LPA."
    LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(USE_SYSTEM_DEPS "Use system-wide installed dependencies (cJSON)" OFF)

set(LPAC_CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${LPAC_CMAKE_MODULE_PATH})

if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  # Git auto-ignore out-of-source build directory
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_compile_options(-Wall -Wextra -Wpedantic)

# Enable LTO when possible.
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

if (USE_SYSTEM_DEPS)
    find_package(cJSON REQUIRED)
elseif (NOT NO_NETWORK)
    set(CJSON_OVERRIDE_BUILD_SHARED_LIBS ON)
    set(CJSON_BUILD_SHARED_LIBS OFF)
    set(ENABLE_CJSON_TEST OFF)
    set(ENABLE_CUSTOM_COMPILER_FLAGS OFF)
    include(FetchContent)
    FetchContent_Declare(
            cjson
            # FIXME: change back to DaveGamble/cJSON if upstream fix it
            # https://github.com/DaveGamble/cJSON/pull/949
            # https://github.com/DaveGamble/cJSON/pull/955
            GIT_REPOSITORY https://github.com/CoelacanthusHex/cJSON
            GIT_TAG 4818f043bda624b5738384eae3a0189b2bd1f5e1
            GIT_PROGRESS ON
            SOURCE_DIR cjson
    )
    FetchContent_MakeAvailable(cjson)
    set(CJSON_LIBRARY cjson)
    # Force cJSON dynamically link CRT although it's statuc librgary.
    set_property(TARGET cjson PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else ()
    message(FATAL_ERROR "cJSON is required. Either set USE_SYSTEM_DEPS to ON or enable network to download it.")
endif ()

add_subdirectory(cjson-ext)
add_subdirectory(euicc)
add_subdirectory(utils)
add_subdirectory(driver)
add_subdirectory(src)
