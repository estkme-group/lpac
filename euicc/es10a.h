#pragma once

#include "euicc.h"

int es10a_get_euicc_configured_addresses(struct euicc_ctx *ctx, char **smdp, char **smds);
int es10a_set_default_dp_address(struct euicc_ctx *ctx, const char *smdp);
