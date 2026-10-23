#include "list.h"
#include "main.h"

#include <euicc/es10c.h>
#include <euicc/tostr.h>
#include <lpac/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Some modems cannot carry a profile list that includes the icon back from the eUICC:
// a Telit FN990A40 over QMI answers InsufficientResources. Asking for every field but
// the icon keeps the response small enough for them.
//
// Only when asked: by default send no tag list at all, exactly as before, and let the
// eUICC return its own set. Requesting every field explicitly could make the response
// larger than it is today on cards whose default set is smaller than that.
#define ENV_SKIP_ICON CUSTOM_ENV_NAME(PROFILE_LIST_SKIP_ICON)

static int applet_main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    _cleanup_es10c_profile_info_list_ struct es10c_profile_info_list *profiles;
    struct es10c_profile_info_list *rptr;
    cJSON *jdata = NULL;
    uint32_t fields = ES10C_PROFILE_INFO_FIELDS_EUICC_DEFAULT;

    if (getenv_or_default(ENV_SKIP_ICON, (bool)false)) {
        fields = ES10C_PROFILE_INFO_FIELDS_ALL & ~(uint32_t)ES10C_PROFILE_INFO_FIELD_ICON;
    }

    if (es10c_get_profiles_info(&euicc_ctx, &profiles, fields)) {
        jprint_error("es10c_get_profiles_info", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    rptr = profiles;

    while (rptr) {
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

    jprint_success(jdata);

    return 0;
}

struct applet_entry applet_profile_list = {
    .name = "list",
    .main = applet_main,
};
