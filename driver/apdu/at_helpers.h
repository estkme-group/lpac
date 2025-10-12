#pragma once

#include "at_cmd.h"

#define AT_WARNING_MESSAGE                                                                 \
    "WARNING: AT DRIVER IS NO LONGER MAINTAINED, IT IS PROVIDED FOR DEMO PURPOSES ONLY.\n" \
    "Some operations (e.g: download, delete, etc.), may fail due to insufficient response time.\n"

char *at_channel_get(struct at_userdata *userdata, int index);
int at_channel_set(struct at_userdata *userdata, int index, const char *identifier);
int at_channel_next_id(struct at_userdata *userdata);
int at_emit_command(struct at_userdata *userdata, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
