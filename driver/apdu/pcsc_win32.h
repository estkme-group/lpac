#pragma once
#ifdef _WIN32
#include <stdint.h>
#include <winscard.h>

char *pcsc_stringify_error_win32(int32_t err);
#endif
