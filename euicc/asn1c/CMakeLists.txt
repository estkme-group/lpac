aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/asn1 LIB_EUICC_ASN1_SRCS)
add_library(euiccasn1 STATIC ${LIB_EUICC_ASN1_SRCS})
target_compile_definitions(euiccasn1 PRIVATE HAVE_CONFIG_H)
target_include_directories(euiccasn1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asn1>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if(MINGW)
    add_subdirectory(mingw32)
    target_link_libraries(euiccasn1 euiccasn1mingw32)
endif()
