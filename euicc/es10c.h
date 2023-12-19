#pragma once

#include "euicc.h"

enum es10c_profile_info_state
{
    ES10C_PROFILE_INFO_STATE_DISABLED = 0,
    ES10C_PROFILE_INFO_STATE_ENABLED = 1,
};

enum es10c_profile_info_class
{
    ES10C_PROFILE_INFO_CLASS_TEST = 0,
    ES10C_PROFILE_INFO_CLASS_PROVISIONING = 1,
    ES10C_PROFILE_INFO_CLASS_OPERATIONAL = 2,
};

enum es10c_icon_type
{
    ES10C_ICON_TYPE_INVALID = -1,
    ES10C_ICON_TYPE_JPEG = 0,
    ES10C_ICON_TYPE_PNG = 1,
};

struct es10c_profile_info
{
    char iccid[(10 * 2) + 1];
    char isdpAid[(16 * 2) + 1];
    char *profileState;
    char *profileClass;
    char *profileNickname;
    char *serviceProviderName;
    char *profileName;
    char *iconType;
    char *icon;
};

int es10c_get_profiles_info(struct euicc_ctx *ctx, struct es10c_profile_info **profiles, int *count);
int es10c_enable_profile_aid(struct euicc_ctx *ctx, const char *aid, int refreshflag);
int es10c_enable_profile_iccid(struct euicc_ctx *ctx, const char *iccid, int refreshflag);
int es10c_disable_profile_aid(struct euicc_ctx *ctx, const char *aid, int refreshflag);
int es10c_disable_profile_iccid(struct euicc_ctx *ctx, const char *iccid, int refreshflag);
int es10c_delete_profile_aid(struct euicc_ctx *ctx, const char *aid);
int es10c_delete_profile_iccid(struct euicc_ctx *ctx, const char *iccid);
int es10c_euicc_memory_reset(struct euicc_ctx *ctx, int op, int tp, int addr);
int es10c_get_eid(struct euicc_ctx *ctx, char **eid);
int es10c_set_nickname(struct euicc_ctx *ctx, const char *iccid, const char *nickname);

void es10c_profile_info_free_all(struct es10c_profile_info *profiles, int count);
void es10c_profile_info_print(struct es10c_profile_info *p);
