aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LIB_EUICC_ASN1_MINGW32_SRCS)
add_library(euiccasn1mingw32 STATIC ${LIB_EUICC_ASN1_MINGW32_SRCS})
target_include_directories(euiccasn1mingw32 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
