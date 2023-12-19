#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

static int applet_main(int argc, char **argv)
{
    struct es10c_profile_info *profiles;
    int profiles_count;
    cJSON *jdata = NULL;

    if (es10c_get_profiles_info(&euicc_ctx, &profiles, &profiles_count))
    {
        jprint_error("es10c_get_profiles_info", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    for (int i = 0; i < profiles_count; i++)
    {
        cJSON *jprofile = NULL;

        jprofile = cJSON_CreateObject();
        cJSON_AddStringOrNullToObject(jprofile, "iccid", profiles[i].iccid);
        cJSON_AddStringOrNullToObject(jprofile, "isdpAid", profiles[i].isdpAid);
        cJSON_AddStringOrNullToObject(jprofile, "profileState", profiles[i].profileState);
        cJSON_AddStringOrNullToObject(jprofile, "profileNickname", profiles[i].profileNickname);
        cJSON_AddStringOrNullToObject(jprofile, "serviceProviderName", profiles[i].serviceProviderName);
        cJSON_AddStringOrNullToObject(jprofile, "profileName", profiles[i].profileName);
        cJSON_AddStringOrNullToObject(jprofile, "iconType", profiles[i].iconType);
        cJSON_AddStringOrNullToObject(jprofile, "icon", profiles[i].icon);
        cJSON_AddStringOrNullToObject(jprofile, "profileClass", profiles[i].profileClass);
        cJSON_AddItemToArray(jdata, jprofile);
    }
    es10c_profile_info_free_all(profiles, profiles_count);

    jprint_success(jdata);

    return 0;
}

struct applet_entry applet_profile_list = {
    .name = "list",
    .main = applet_main,
};
