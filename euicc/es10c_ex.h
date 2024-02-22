#pragma once

#include "euicc.h"

struct es10c_ex_euiccinfo2
{
    char *profileVersion;
    char *svn;
    char *euiccFirmwareVer;
    struct
    {
        uint32_t installedApplication;
        uint32_t freeNonVolatileMemory;
        uint32_t freeVolatileMemory;
    } extCardResource;
    const char **uiccCapability;
    char *javacardVersion;
    char *globalplatformVersion;
    const char **rspCapability;
    char **euiccCiPKIdListForVerification;
    char **euiccCiPKIdListForSigning;
    const char *euiccCategory;
    const char **forbiddenProfilePolicyRules;
    char *ppVersion;
    char *sasAcreditationNumber;
    struct
    {
        char *platformLabel;
        char *discoveryBaseURL;
    } certificationDataObject;
};

int es10c_ex_get_euiccinfo2(struct euicc_ctx *ctx, struct es10c_ex_euiccinfo2 *euiccinfo2);
void es10c_ex_euiccinfo2_free(struct es10c_ex_euiccinfo2 *euiccinfo2);
