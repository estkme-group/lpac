#pragma once

#include "euicc.h"

struct es10a_EuiccConfiguredAddresses
{
    char *defaultDpAddress;
    char *rootDsAddress;
};

int es10a_GetEuiccConfiguredAddresses(struct euicc_ctx *ctx, struct es10a_EuiccConfiguredAddresses *address);
int es10a_SetDefaultDpAddress(struct euicc_ctx *ctx, const char *smdp);

void es10a_euicc_configured_addresses_free(struct es10a_EuiccConfiguredAddresses *address);
