add_library(cjson-ext OBJECT cjson-ext/cJSON_ex.c)
target_include_directories(cjson-ext PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cjson-ext PRIVATE ${CJSON_LIBRARY})
target_compile_options(cjson-ext PRIVATE -Wall -Wextra)
