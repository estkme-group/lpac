#pragma once
#include "euicc.h"
#include "es10c.h"

struct es8p_metadata
{
    char iccid[(10 * 2) + 1];
    char *serviceProviderName;
    char *profileName;
    enum es10c_icon_type iconType;
    char *icon;
    enum es10c_profile_class profileClass;
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
};

int es8p_metadata_parse(struct es8p_metadata **metadata, const char *b64_Metadata);
void es8p_metadata_free(struct es8p_metadata **stru_metadata);

struct es8p_smdp_signed2
{
    long confirmationCodeRequired;
};

int es8p_smdp_signed2_parse(struct es8p_smdp_signed2 **smdp_signed2, const char *b64_smdpSigned2);
void es8p_smdp_signed2_free(struct es8p_smdp_signed2 **smdp_signed2);
