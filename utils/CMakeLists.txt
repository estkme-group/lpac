if (LPAC_DYNAMIC_DRIVERS)
    add_library(lpac-utils SHARED lpac/utils.c)
    install(TARGETS lpac-utils LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
else()
    add_library(lpac-utils OBJECT lpac/utils.c)
endif()
target_link_libraries(lpac-utils PRIVATE ${CJSON_LIBRARY} euicc)
target_compile_options(lpac-utils PRIVATE -Wall -Wextra)
target_sources(
    lpac-utils
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            lpac/utils.h
)
