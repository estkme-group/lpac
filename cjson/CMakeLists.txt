include_directories(.)
aux_source_directory(. LIB_CJSON_SRCS)
add_library(cjson-static STATIC ${LIB_CJSON_SRCS})
