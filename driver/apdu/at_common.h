#pragma once

#include "at_cmd.h"
#include "driver.h"

#include <stdbool.h>

int at_emit_command(struct at_userdata *userdata, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

bool at_test_commands(struct at_userdata *userdata, const char **commands);

void at_interface_disconnect(struct euicc_ctx *ctx);

int at_interface_main_entry(const struct euicc_apdu_interface *ifstruct, int argc, char **argv);

void at_interface_finished(struct euicc_apdu_interface *ifstruct);
