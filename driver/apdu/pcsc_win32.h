#pragma once
#ifdef _WIN32
#include <stdint.h>
#include <winscard.h>

char *pcsc_stringify_error(int32_t err);
#endif
