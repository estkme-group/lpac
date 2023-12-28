option(LINUX_MINGW32 "Build for windows on Linux" OFF)

if (APPLE)
    message(STATUS "Building for macOS")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    set(CMAKE_SYSTEM_NAME Darwin)
endif()

if(LINUX_MINGW32)
    set(CMAKE_SYSTEM_NAME Windows)

    set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
    set(CMAKE_Fortran_COMPILER ${TOOLCHAIN_PREFIX}-gfortran)
    set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

    set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

if(CYGWIN)
    set(CMAKE_SYSTEM_NAME Windows)
    message(STATUS "Building for Cygwin")
    add_definitions(-DHAVE_TIMEGM=1)
    add_definitions(-DWIN32)
endif()

cmake_minimum_required (VERSION 3.8)
project (lpac)


include_directories(.)
add_subdirectory(cjson)
add_subdirectory(euicc)
add_subdirectory(interface)
add_subdirectory(src)
if(LINUX_MINGW32)
    add_subdirectory(dlfcn-win32)
endif()
