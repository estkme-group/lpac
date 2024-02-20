aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LIB_EUICC_SRCS)
add_library(euicc STATIC ${LIB_EUICC_SRCS})
target_link_libraries(euicc cjson-static)
target_include_directories(euicc PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
