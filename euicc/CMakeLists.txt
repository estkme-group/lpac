option(LPAC_DYNAMIC_LIBEUICC "Build and install libeuicc as a dynamic library" OFF)
option(LIBEUICC_REDUCED_STDLIB_CALL "Reduce standard library calling" OFF)

if(LPAC_DYNAMIC_LIBEUICC)
    add_library(euicc SHARED)
else()
    add_library(euicc STATIC)
endif()
target_link_libraries(euicc ${CJSON_LIBRARY} cjson-ext)
target_sources(
    euicc
    PRIVATE
        base64.c
        derutil.c
        es8p.c
        es9p.c
        es9p_errors.c
        es10a.c
        es10b.c
        es10c.c
        es10c_ex.c
        euicc.c
        hexutil.c
        interface.c
        logger.c
        sha256.c
        tostr.c
)
target_sources(
    euicc
    PRIVATE
        FILE_SET private_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES
            euicc.private.h
            interface.private.h
)
target_sources(
    euicc
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES
            base64.h
            es8p.h
            es9p_errors.h
            es10b.h
            es10c_ex.h
            interface.h
            logger.h
            tostr.h
            derutil.h
            es9p.h
            es10a.h
            es10c.h
            euicc.h
            hexutil.h
            sha256.h

)

target_compile_options(euicc PRIVATE -Wall -Wextra -Wvla)
set_target_properties(euicc PROPERTIES
        # follow C89, but allow for loop initial declarations
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS ON
)
if(LPAC_DYNAMIC_LIBEUICC)
    # Only useful on Windows, and will lead to invalid arguments on ld.gold.
    if(WIN32)
        target_link_options(euicc PRIVATE "LINKER:--export-all-symbols")
    endif()
    # Install a pkg-config file
    configure_file(libeuicc.pc.in libeuicc.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libeuicc.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    # Configure libeuicc.so installation
    set_target_properties(euicc PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS euicc   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                            FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if (LIBEUICC_REDUCED_STDLIB_CALL)
    target_compile_definitions(euicc PRIVATE LIBEUICC_REDUCED_STDLIB_CALL)
endif ()
