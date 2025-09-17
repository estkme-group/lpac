aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LIB_DLFCN_SRCS)
add_library(dlfcn-win32 STATIC ${LIB_DLFCN_SRCS})
target_sources(
    dlfcn-win32
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES
            dlfcn.h
)
