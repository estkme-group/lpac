#include "info.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

static int applet_main(int argc, char **argv)
{
    char *eid = NULL;
    char *default_smdp = NULL;
    char *default_smds = NULL;
    struct es10cex_euiccinfo2 *euiccinfo2 = (struct es10cex_euiccinfo2*)malloc(sizeof(struct es10cex_euiccinfo2));
    cJSON *jeuiccinfo2 = NULL;
    cJSON *jdata = NULL;

    if (es10c_get_eid(&euicc_ctx, &eid))
    {
        jprint_error("es10c_get_eid", NULL);
        return -1;
    }

    if (es10a_get_euicc_configured_addresses(&euicc_ctx, &default_smdp, &default_smds))
    {
        jprint_error("es10a_get_euicc_configured_addresses", NULL);
        return -1;
    }

    if (es10cex_get_euiccinfo2(&euicc_ctx, euiccinfo2) == 0)
    {
        jeuiccinfo2 = cJSON_CreateObject();
    }

    jdata = cJSON_CreateObject();
    cJSON_AddStringOrNullToObject(jdata, "eid", eid);
    cJSON_AddStringOrNullToObject(jdata, "default_smds", default_smds);
    cJSON_AddStringOrNullToObject(jdata, "default_smdp", default_smdp);
    if (jeuiccinfo2)
    {
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "profile_version", euiccinfo2->profile_version);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "sgp22_version", euiccinfo2->sgp22_version);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "euicc_firmware_version", euiccinfo2->euicc_firmware_version);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "uicc_firmware_version", euiccinfo2->uicc_firmware_version);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "global_platform_version", euiccinfo2->global_platform_version);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "protection_profile_version", euiccinfo2->pp_version);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "sas_accreditation_number", euiccinfo2->sas_accreditation_number);
        cJSON_AddNumberToObject(jeuiccinfo2, "free_nvram", euiccinfo2->free_nvram);
        cJSON_AddNumberToObject(jeuiccinfo2, "free_ram", euiccinfo2->free_ram);
        if (euiccinfo2->euicc_ci_public_key_id_list_for_verification.count)
        {
            cJSON_AddItemToObject(jeuiccinfo2, "euicc_ci_public_key_id_list_for_verification",
                    cJSON_CreateStringArray((const char * const*)euiccinfo2->euicc_ci_public_key_id_list_for_verification.list,
                        euiccinfo2->euicc_ci_public_key_id_list_for_verification.count));
        }
        else
        {
            cJSON_AddNullToObject(jeuiccinfo2, "euicc_ci_public_key_id_list_for_verification");
        }
        if (euiccinfo2->euicc_ci_public_key_id_list_for_signing.count)
        {
            cJSON_AddItemToObject(jeuiccinfo2, "euicc_ci_public_key_id_list_for_signing",
                    cJSON_CreateStringArray((const char * const*)euiccinfo2->euicc_ci_public_key_id_list_for_signing.list,
                        euiccinfo2->euicc_ci_public_key_id_list_for_signing.count));
        }
        else
        {
            cJSON_AddNullToObject(jeuiccinfo2, "euicc_ci_public_key_id_list_for_signing");
        }
    }
    cJSON_AddItemToObject(jdata, "euiccinfo2", jeuiccinfo2);

    jprint_success(jdata);

    free(eid);
    free(default_smdp);
    free(default_smds);
    es10cex_free_euiccinfo2(euiccinfo2);

    return 0;
}

struct applet_entry applet_chip_info = {
    .name = "info",
    .main = applet_main,
};
