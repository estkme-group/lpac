#pragma once

#include <cjson/cJSON.h>

#include <stdio.h>

int afgets(char **obuf, FILE *fp);

int receive_payload(FILE *fp, const char *expected_type, cJSON **payload);
