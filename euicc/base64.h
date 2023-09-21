#pragma once

int euicc_base64_decode_len(const char *bufcoded);
int euicc_base64_decode(char *bufplain, const char *bufcoded);
int euicc_base64_encode_len(int len);
int euicc_base64_encode(char *encoded, const char *string, int len);
