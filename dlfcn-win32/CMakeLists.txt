include_directories(.)
aux_source_directory(. LIB_DLFCN_SRCS)
add_library(dlfcn-win32 STATIC ${LIB_DLFCN_SRCS})
