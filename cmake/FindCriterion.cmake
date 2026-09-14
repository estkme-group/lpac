find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_CRITERION criterion)

find_path(CRITERION_INCLUDE_DIR
    NAMES criterion/criterion.h
    HINTS ${PC_CRITERION_INCLUDEDIR}
          ${PC_CRITERION_INCLUDE_DIRS}
          ${CMAKE_INSTALL_PREFIX}/include
)
find_library(CRITERION_LIBRARY NAMES criterion libcriterion
    HINTS ${PC_CRITERION_LIBDIR}
          ${PC_CRITERION_LIBRARY_DIRS}
          ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Criterion DEFAULT_MSG CRITERION_LIBRARY CRITERION_INCLUDE_DIR)

mark_as_advanced(CRITERION_LIBRARY CRITERION_INCLUDE_DIR)

if(CRITERION_FOUND AND NOT TARGET Criterion::Criterion)
    add_library(Criterion::Criterion UNKNOWN IMPORTED)
    set_target_properties(Criterion::Criterion PROPERTIES
        IMPORTED_LOCATION "${CRITERION_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CRITERION_INCLUDE_DIR}"
        INTERFACE_COMPILE_OPTIONS "${PC_CRITERION_CFLAGS_OTHER}"
    )
endif()
