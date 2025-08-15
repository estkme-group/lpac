#pragma once

#include "euicc/euicc.h"

#include <stdbool.h>

struct euicc_logger *build_euicc_logger(FILE *fp, bool apdu_debug, bool http_debug);
