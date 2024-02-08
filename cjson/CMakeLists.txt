aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LIB_CJSON_SRCS)
add_library(cjson-static STATIC ${LIB_CJSON_SRCS})
target_include_directories(cjson-static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
