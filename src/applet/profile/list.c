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
        cJSON_AddStringToObject(jprofile, "iccid", profiles[i].iccid);
        cJSON_AddStringToObject(jprofile, "isdpAid", profiles[i].isdpAid);
        cJSON_AddStringToObject(jprofile, "profileState", profiles[i].profileState);
        cJSON_AddStringToObject(jprofile, "profileNickname", profiles[i].profileNickname);
        cJSON_AddStringToObject(jprofile, "serviceProviderName", profiles[i].serviceProviderName);
        cJSON_AddStringToObject(jprofile, "profileName", profiles[i].profileName);
        cJSON_AddStringToObject(jprofile, "iconType", profiles[i].iconType);
        cJSON_AddStringToObject(jprofile, "icon", profiles[i].icon);
        cJSON_AddStringToObject(jprofile, "profileClass", profiles[i].profileClass);
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
