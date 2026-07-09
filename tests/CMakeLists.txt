find_package(Criterion REQUIRED)

if (NOT LPAC_BUILD_LIBEUICC_ONLY)
    add_executable(lpac-utils-tests utils_test.c)
    target_compile_options(lpac-utils-tests PRIVATE -Wall -Wextra -Wconversion)
    target_link_libraries(lpac-utils-tests PRIVATE lpac-utils Criterion::Criterion)

    add_executable(lpac-tests applet_test.c)
    target_compile_options(lpac-tests PRIVATE -Wall -Wextra -Wconversion)
    target_link_libraries(lpac-tests PRIVATE lpac-lib lpac-internal Criterion::Criterion)

    add_test(NAME lpac-utils-tests COMMAND lpac-utils-tests)
    add_test(NAME lpac-tests COMMAND lpac-tests)
endif ()

add_executable(libeuicc-tests libeuicc_test.c)
target_compile_options(libeuicc-tests PRIVATE -Wall -Wextra -Wconversion)
target_link_libraries(libeuicc-tests PRIVATE euicc euicc-internal Criterion::Criterion)

add_test(NAME libeuicc-tests COMMAND libeuicc-tests)

