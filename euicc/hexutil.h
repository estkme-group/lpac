#pragma once

#include <stdint.h>

int euicc_hexutil_bin2hex(char *restrict output, uint32_t output_len, const uint8_t *restrict bin, uint32_t bin_len);

int euicc_hexutil_hex2bin(uint8_t *restrict output, uint32_t output_len, const char *restrict input);

int euicc_hexutil_hex2bin_r(uint8_t *restrict output, uint32_t output_len, const char *restrict input,
                            uint32_t input_len);

int euicc_hexutil_gsmbcd2bin(uint8_t *restrict output, uint32_t output_len, const char *restrict input,
                             uint32_t padding_to);

int euicc_hexutil_bin2gsmbcd(char *restrict output, uint32_t output_len, const uint8_t *restrict bin, uint32_t bin_len);
