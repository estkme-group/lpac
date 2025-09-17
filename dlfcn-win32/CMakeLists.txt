add_library(dlfcn-win32 STATIC)
target_sources(
    dlfcn-win32
    PRIVATE
        dlfcn.c
)
target_sources(
    dlfcn-win32
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES
            dlfcn.h
)
