#pragma once
#ifdef _WIN32
#include <winscard.h>

const char *pcsc_stringify_error(LONG);
#endif
