aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LIB_DLFCN_SRCS)
add_library(dlfcn-win32 STATIC ${LIB_DLFCN_SRCS})
target_include_directories(dlfcn-win32 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
