#pragma once

#include "euicc.h"

struct es10c_profile_info
{
    char iccid[(10 * 2) + 1];
    char isdpAid[(16 * 2) + 1];
    const char *profileState;
    const char *profileClass;
    char *profileNickname;
    char *serviceProviderName;
    char *profileName;
    const char *iconType;
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

    struct es10c_profile_info *next;
};

int es10c_get_profiles_info(struct euicc_ctx *ctx, struct es10c_profile_info **profiles);
int es10c_enable_profile_aid(struct euicc_ctx *ctx, const char *aid, char refreshflag);
int es10c_enable_profile_iccid(struct euicc_ctx *ctx, const char *iccid, char refreshflag);
int es10c_disable_profile_aid(struct euicc_ctx *ctx, const char *aid, char refreshflag);
int es10c_disable_profile_iccid(struct euicc_ctx *ctx, const char *iccid, char refreshflag);
int es10c_delete_profile_aid(struct euicc_ctx *ctx, const char *aid);
int es10c_delete_profile_iccid(struct euicc_ctx *ctx, const char *iccid);
int es10c_euicc_memory_reset(struct euicc_ctx *ctx);
int es10c_get_eid(struct euicc_ctx *ctx, char **eid);
int es10c_set_nickname(struct euicc_ctx *ctx, const char *iccid, const char *nickname);

void es10c_profile_info_free_all(struct es10c_profile_info *profiles);
