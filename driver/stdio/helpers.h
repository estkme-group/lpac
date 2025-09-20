#pragma once

#include <cjson/cJSON.h>

#include <stdio.h>

int receive_payload(FILE *fp, const char *expected_type, cJSON **payload);
