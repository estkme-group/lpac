add_library(cjson-ext OBJECT cjson-ext/cJSON_ex.c)
target_link_libraries(cjson-ext PRIVATE ${CJSON_LIBRARY})
target_compile_options(cjson-ext PRIVATE -Wall -Wextra)
target_sources(
    cjson-ext
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            cjson-ext/cJSON_ex.h
)
