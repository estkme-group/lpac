add_library(lpac-utils SHARED lpac/utils.c)
install(TARGETS lpac-utils LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
target_link_libraries(lpac-utils PRIVATE ${CJSON_LIBRARY} euicc)
target_compile_options(lpac-utils PRIVATE -Wall -Wextra)

if(WIN32)
    # Only useful on Windows, and will lead to invalid arguments on ld.gold.
    target_link_options(lpac-utils PRIVATE "LINKER:--export-all-symbols")
    # FIXME: Glibc since 2.17 have move clock_* from librt to libc so
    #        single-thread program can use these functions without the
    #        overheads associated with multi-thread support.
    #        But MinGW doesn't.
    find_package(Threads REQUIRED)
    target_link_libraries(lpac-utils PRIVATE Threads::Threads)
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
