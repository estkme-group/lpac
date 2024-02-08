cmake_minimum_required (VERSION 3.8)
project (lpac)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
    endif
endif()

add_subdirectory(cjson)
add_subdirectory(euicc)
add_subdirectory(interface)
add_subdirectory(src)
if(MINGW)
    add_subdirectory(dlfcn-win32)
    set(DL_LIBRARY dlfcn-win32)
else()
    set(DL_LIBRARY dl)
endif()
