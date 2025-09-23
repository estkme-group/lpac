cmake_minimum_required (VERSION 3.23)

project (lpac
    VERSION 2.3.0
    HOMEPAGE_URL "https://github.com/estkme-group/lpac"
    DESCRIPTION "C-based eUICC LPA."
    LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CMakeDependentOption)
option(USE_SYSTEM_DEPS "Use system-wide installed dependencies" OFF)
option(STANDALONE_MODE "All things in a relocatable directory" OFF)
option(LPAC_DYNAMIC_LIBEUICC "Build and install libeuicc as a dynamic library" ON)


set(LPAC_CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${LPAC_CMAKE_MODULE_PATH})

if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  # Git auto-ignore out-of-source build directory
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_compile_options(-Wall -Wextra -Wpedantic)

if (NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(CHECK_START "Detecting LTO")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT CMAKE_INTERPROCEDURAL_OPTIMIZATION OUTPUT LTO_ERROR)
    if (LTO_ERROR)
        message(CHECK_FAIL "Not supported: ${LTO_ERROR}")
    else ()
        message(CHECK_PASS "Supported")
    endif ()
endif ()

if (NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    message(CHECK_START "Detecting PIE/PIC")
    include(CheckPIESupported)
    check_pie_supported(OUTPUT_VARIABLE PIC_ERROR)
    set(CMAKE_POSITION_INDEPENDENT_CODE ${CMAKE_C_LINK_PIE_SUPPORTED})
    if (PIC_ERROR)
        message(CHECK_FAIL "Not supported: ${PIC_ERROR}")
    else ()
        message(CHECK_PASS "Supported")
    endif ()
endif ()

if (APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif ()

if(APPLE)
    set(RPATH_BINARY_PATH "@loader_path")
else()
    set(RPATH_BINARY_PATH "$ORIGIN")
endif()

if(NOT CMAKE_INSTALL_RPATH)
    if (STANDALONE_MODE)
        cmake_policy(SET CMP0177 NEW)
        set(CMAKE_INSTALL_RPATH "${RPATH_BINARY_PATH}:${RPATH_BINARY_PATH}/lib")
        set(CMAKE_INSTALL_PREFIX "")
        set(CMAKE_INSTALL_BINDIR "executables")
        set(CMAKE_INSTALL_LIBDIR "executables/lib")
        include(GNUInstallDirs)
        set(DRIVER_INSTALL_PATH ${CMAKE_INSTALL_FULL_BINDIR}/driver)
    elseif (UNIX)
        include(GNUInstallDirs)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}/lpac")
        set(DRIVER_INSTALL_PATH "${CMAKE_INSTALL_FULL_LIBDIR}/lpac/driver")
    endif()
endif()

if(WIN32)
    add_subdirectory(dlfcn-win32)
    set(DL_LIBRARY dlfcn-win32)
else()
    set(DL_LIBRARY dl)
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
