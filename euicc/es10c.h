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

int es10c_GetProfilesInfo(struct euicc_ctx *ctx, struct es10c_profile_info **profiles);
int es10c_EnableProfile(struct euicc_ctx *ctx, const char *id, unsigned char refreshFlag);
int es10c_DisableProfile(struct euicc_ctx *ctx, const char *id, unsigned char refreshFlag);
int es10c_DeleteProfile(struct euicc_ctx *ctx, const char *id);
int es10c_eUICCMemoryReset(struct euicc_ctx *ctx);
int es10c_GetEID(struct euicc_ctx *ctx, char **eid);
int es10c_SetNickname(struct euicc_ctx *ctx, const char *iccid, const char *nickname);

void es10c_profile_info_free_all(struct es10c_profile_info *profiles);
