#pragma once
#include "euicc_export.h"

#include "euicc.h"

struct es10a_euicc_configured_addresses {
    char *defaultDpAddress;
    char *rootDsAddress;
};

EUICC_API int es10a_get_euicc_configured_addresses(struct euicc_ctx *ctx,
                                                   struct es10a_euicc_configured_addresses *address);
EUICC_API int es10a_set_default_dp_address(struct euicc_ctx *ctx, const char *smdp);

EUICC_API void es10a_euicc_configured_addresses_free(struct es10a_euicc_configured_addresses *address);
