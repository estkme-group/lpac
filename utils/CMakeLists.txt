add_library(lpac-utils SHARED lpac/utils.c)
install(TARGETS lpac-utils LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
target_link_libraries(lpac-utils PRIVATE ${CJSON_LIBRARY} euicc)
target_compile_options(lpac-utils PRIVATE -Wall -Wextra)

# Only useful on Windows, and will lead to invalid arguments on ld.gold.
if(WIN32)
    target_link_options(lpac-utils PRIVATE "LINKER:--export-all-symbols")
endif()

target_sources(
    lpac-utils
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            lpac/utils.h
            lpac/list.h
)
