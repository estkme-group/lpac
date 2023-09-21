#pragma once

int euicc_hexutil_bin2hex(char *output, unsigned output_len, const char *binData, int length);
int euicc_hexutil_hex2bin(char *output, unsigned output_len, const char *hexStr);
int euicc_hexutil_hex2bin_r(char *output, unsigned output_len, const char *str, unsigned str_len);
int euicc_hexutil_gsmbcd2bin(char *output, unsigned output_len, const char *str);
int euicc_hexutil_bin2gsmbcd(char *output, unsigned output_len, const char *binData, int length);
