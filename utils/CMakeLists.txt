add_library(lpac-utils OBJECT lpac/utils.c)
target_include_directories(lpac-utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lpac-utils PRIVATE ${CJSON_LIBRARY} euicc)
target_compile_options(lpac-utils PRIVATE -Wall -Wextra)
