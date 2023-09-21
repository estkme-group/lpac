include_directories(.)
aux_source_directory(. LIB_EUICC_ASN1_MINGW32_SRCS)
add_library (euiccasn1mingw32 STATIC ${LIB_EUICC_ASN1_MINGW32_SRCS})
