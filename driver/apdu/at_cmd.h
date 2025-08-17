#pragma once

#include <cjson/cJSON.h>

#define AT_BUFFER_SIZE 20480
#define AT_READ_BUFFER_SIZE 4096

#define ENV_AT_DEBUG APDU_ENV_NAME(AT, DEBUG)
#define ENV_AT_DEVICE APDU_ENV_NAME(AT, DEVICE)

struct at_userdata;

extern int (*enumerate_serial_device)(cJSON *);

char *get_at_default_device(struct at_userdata *userdata);

int at_write_command(struct at_userdata *userdata, const char *command);

int at_expect(struct at_userdata *userdata, char **response, const char *expected);

int at_device_open(struct at_userdata *userdata, const char *device_name);

int at_device_close(struct at_userdata *userdata);

int at_setup_userdata(struct at_userdata **userdata);

void at_cleanup_userdata(struct at_userdata **userdata);
