#pragma once

#include "at_cmd.h"

char *at_channel_get(struct at_userdata *userdata, int index);
int at_channel_set(struct at_userdata *userdata, int index, const char *identifier);
int at_channel_next_id(struct at_userdata *userdata);
int at_emit_command(struct at_userdata *userdata, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
