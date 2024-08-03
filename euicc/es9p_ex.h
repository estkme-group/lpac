#include "euicc.h"

enum es9p_ex_icon_type
{
    ES9P_EX_ICON_TYPE_NULL = -1,
    ES9P_EX_ICON_TYPE_JPEG = 0,
    ES9P_EX_ICON_TYPE_PNG = 1,
    ES9P_EX_ICON_TYPE_UNDEFINED = 255,
};

enum es9p_ex_profile_class
{
    ES9P_EX_PROFILE_CLASS_NULL = -1,
    ES9P_EX_PROFILE_CLASS_TEST = 0,
    ES9P_EX_PROFILE_CLASS_PROVISIONING = 1,
    ES9P_EX_PROFILE_CLASS_OPERATIONAL = 2,
    ES9P_EX_PROFILE_CLASS_UNDEFINED = 255,
};

struct es9p_ex_profile_metadata
{
    char iccid[(10 * 2) + 1];
    char isdpAid[(16 * 2) + 1];
    char *serviceProviderName;
    char *profileName;
    enum es9p_ex_profile_class profileClass;
    enum es9p_ex_icon_type iconType;
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
};

int es9p_ex_get_profile_metadata(struct euicc_ctx *ctx, struct es9p_ex_profile_metadata *profile_metadata);
void es9p_ex_free(struct es9p_ex_profile_metadata *profile_metadata);
