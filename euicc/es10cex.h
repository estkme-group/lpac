#pragma once

#include "euicc.h"

struct es10cex_euicc_info {
    char *profile_version;
    char *sgp22_version;
    char *euicc_firmware_version;
    char *uicc_firmware_version;
    char *global_platform_version;
    char *sas_accreditation_number;
    char *pp_version;
    uint8_t installed_app;
    uint32_t free_nvram;
    uint32_t free_ram;
};

int es10cex_get_euicc_info(struct euicc_ctx *ctx, struct es10cex_euicc_info *info);

void es10cex_euicc_info_free(struct es10cex_euicc_info *info);
