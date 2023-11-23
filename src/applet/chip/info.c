#include "info.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

static int main(int argc, char **argv)
{
    char *eid = NULL;
    char *default_smdp = NULL;
    char *default_smds = NULL;
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

    jdata = cJSON_CreateObject();
    cJSON_AddStringToObject(jdata, "eid", eid);
    cJSON_AddStringToObject(jdata, "default_smds", default_smds);
    cJSON_AddStringToObject(jdata, "default_smdp", default_smdp);
    jprint_success(jdata);

    free(eid);
    free(default_smdp);
    free(default_smds);

    return 0;
}

struct applet_entry applet_chip_info = {
    .name = "info",
    .main = main,
};
