#pragma once
#include <inttypes.h>

int euicc_hexutil_hex2bin_r(uint8_t *output, uint32_t output_len, const char *str, uint32_t str_len);
int euicc_hexutil_hex2bin(uint8_t *output, uint32_t output_len, const char *str);
int euicc_hexutil_bin2hex(char *output, uint32_t output_len, const uint8_t *bin, uint32_t bin_len);
int euicc_hexutil_gsmbcd2bin(uint8_t *output, uint32_t output_len, const char *str, uint32_t padding_to);
int euicc_hexutil_bin2gsmbcd(char *output, uint32_t output_len, const uint8_t *binData, uint32_t length);
