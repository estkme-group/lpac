#pragma once

#include "euicc.h"

struct es10cex_euiccinfo2
{
    char profile_version[16];
    char sgp22_version[16];
    char euicc_firmware_version[16];
    char uicc_firmware_version[16];
    char global_platform_version[16];
    char sas_accreditation_number[65];
    char pp_version[16];
    int free_nvram;
    int free_ram;
};

int es10cex_get_euiccinfo2(struct euicc_ctx *ctx, struct es10cex_euiccinfo2 *info);
