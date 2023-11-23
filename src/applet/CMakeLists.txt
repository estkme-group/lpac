include_directories(.)
aux_source_directory(. LPAC_APPLET_SRCS)
aux_source_directory(chip LPAC_APPLET_SRCS)
aux_source_directory(profile LPAC_APPLET_SRCS)
aux_source_directory(notification LPAC_APPLET_SRCS)

add_library (lpac-applet STATIC ${LPAC_APPLET_SRCS})
target_link_libraries(lpac-applet euicc)
