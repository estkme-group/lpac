#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10c.h>
#include <euicc/tostr.h>

static int applet_main(int argc, char **argv)
{
    struct es10c_profile_info_list *profiles, *rptr;
    cJSON *jdata = NULL;

    if (es10c_get_profiles_info(&euicc_ctx, &profiles))
    {
        jprint_error("es10c_get_profiles_info", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    rptr = profiles;

    while (rptr)
    {
        cJSON *jprofile = NULL;

        jprofile = cJSON_CreateObject();
        cJSON_AddStringOrNullToObject(jprofile, "iccid", rptr->iccid);
        cJSON_AddStringOrNullToObject(jprofile, "isdpAid", rptr->isdpAid);
        cJSON_AddStringOrNullToObject(jprofile, "profileState", euicc_profilestate2str(rptr->profileState));
        cJSON_AddStringOrNullToObject(jprofile, "profileNickname", rptr->profileNickname);
        cJSON_AddStringOrNullToObject(jprofile, "serviceProviderName", rptr->serviceProviderName);
        cJSON_AddStringOrNullToObject(jprofile, "profileName", rptr->profileName);
        cJSON_AddStringOrNullToObject(jprofile, "iconType", euicc_icontype2str(rptr->iconType));
        cJSON_AddStringOrNullToObject(jprofile, "icon", rptr->icon);
        cJSON_AddStringOrNullToObject(jprofile, "profileClass", euicc_profileclass2str(rptr->profileClass));
        cJSON_AddItemToArray(jdata, jprofile);

        rptr = rptr->next;
    }
    es10c_profile_info_list_free_all(profiles);

    jprint_success(jdata);

    return 0;
}

struct applet_entry applet_profile_list = {
    .name = "list",
    .main = applet_main,
};
