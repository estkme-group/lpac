find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PCSCLITE libpcsclite)

find_path(PCSCLITE_INCLUDE_DIR
  NAMES winscard.h pcsclite.h wintypes.h debuglog.h ifdhandler.h reader.h
  HINTS ${PC_PCSCLITE_INCLUDEDIR}
        ${PC_PCSCLITE_INCLUDE_DIRS}
        ${PC_PCSCLITE_INCLUDE_DIRS}/PCSC
        ${CMAKE_INSTALL_PREFIX}/include
)
find_library(PCSCLITE_LIBRARIES NAMES pcsclite libpcsclite PCSC
  HINTS ${PC_PCSCLITE_LIBDIR}
        ${PC_PCSCLITE_LIBRARY_DIRS}
        ${CMAKE_INSTALL_PREFIX}/lib
        ${CMAKE_INSTALL_PREFIX}/lib64
)

# handle the QUIETLY and REQUIRED arguments and set PCSCLITE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCSCLite DEFAULT_MSG PCSCLITE_LIBRARIES PCSCLITE_INCLUDE_DIR)

mark_as_advanced(PCSCLITE_LIBRARIES PCSCLITE_INCLUDE_DIR)

if(PCSCLITE_FOUND AND NOT TARGET PCSCLite::PCSCLite)
  add_library(PCSCLite::PCSCLite UNKNOWN IMPORTED)
  set_target_properties(PCSCLite::PCSCLite PROPERTIES
    IMPORTED_LOCATION "${PCSCLITE_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${PCSCLITE_INCLUDE_DIR}"
  )
endif()
