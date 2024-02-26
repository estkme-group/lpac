#pragma once

#include "euicc.h"

enum es10c_profile_state
{
    ES10C_PROFILE_STATE_NULL = -1,
    ES10C_PROFILE_STATE_DISABLED = 0,
    ES10C_PROFILE_STATE_ENABLED = 1,
    ES10C_PROFILE_STATE_UNDEFINED = 255,
};

enum es10c_profile_class
{
    ES10C_PROFILE_CLASS_NULL = -1,
    ES10C_PROFILE_CLASS_TEST = 0,
    ES10C_PROFILE_CLASS_PROVISIONING = 1,
    ES10C_PROFILE_CLASS_OPERATIONAL = 2,
    ES10C_PROFILE_CLASS_UNDEFINED = 255,
};

enum es10c_icon_type
{
    ES10C_ICON_TYPE_NULL = -1,
    ES10C_ICON_TYPE_JPEG = 0,
    ES10C_ICON_TYPE_PNG = 1,
    ES10C_ICON_TYPE_UNDEFINED = 255,
};

struct es10c_profile_info_list
{
    char iccid[(10 * 2) + 1];
    char isdpAid[(16 * 2) + 1];
    enum es10c_profile_state profileState;
    enum es10c_profile_class profileClass;
    char *profileNickname;
    char *serviceProviderName;
    char *profileName;
    enum es10c_icon_type iconType;
    char *icon;
    struct
    {
        char **profileManagementOperation;
        char *notificationAddress;
    } notificationConfigurationInfo;
    struct
    {
        char *mccmnc;
        char *gid1;
        char *gid2;
    } profileOwner;
    struct
    {
        char *dpOid;
    } dpProprietaryData;
    char **profilePolicyRules;

    struct es10c_profile_info_list *next;
};

int es10c_get_profiles_info(struct euicc_ctx *ctx, struct es10c_profile_info_list **profileInfoList);
int es10c_enable_profile(struct euicc_ctx *ctx, const char *id, uint8_t refreshFlag);
int es10c_disable_profile(struct euicc_ctx *ctx, const char *id, uint8_t refreshFlag);
int es10c_delete_profile(struct euicc_ctx *ctx, const char *id);
int es10c_euicc_memory_reset(struct euicc_ctx *ctx);
int es10c_get_eid(struct euicc_ctx *ctx, char **eidValue);
int es10c_set_nickname(struct euicc_ctx *ctx, const char *iccid, const char *profileNickname);

void es10c_profile_info_list_free_all(struct es10c_profile_info_list *profileInfoList);
