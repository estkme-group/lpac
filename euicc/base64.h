#pragma once

#include "euicc_export.h"

EUICC_API int euicc_base64_decode_len(const char *bufcoded);
EUICC_API int euicc_base64_decode(unsigned char *bufplain, const char *bufcoded);
EUICC_API int euicc_base64_encode_len(int len);
EUICC_API int euicc_base64_encode(char *encoded, const unsigned char *string, int len);
