#pragma once

#include <stdbool.h>
#include <helpers.h>

#define AT_BUFFER_SIZE 20480
#define AT_READ_BUFFER_SIZE 4096

#define ENV_AT_DEBUG APDU_ENV_NAME(AT, DEBUG)
#define ENV_AT_DEVICE APDU_ENV_NAME(AT, DEVICE)

bool jprint_enumerate_devices(cJSON *data);
