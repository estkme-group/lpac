#pragma once

#include "euicc.h"

struct es10c_ex_euiccinfo2
{
    char profileVersion[16];
    char svn[16];
    char euiccFirmwareVer[16];
    struct
    {
        unsigned int installedApplication;
        unsigned int freeNonVolatileMemory;
        unsigned int freeVolatileMemory;
    } extCardResource;
    const char **uiccCapability;
    char javacardVersion[16];
    char globalplatformVersion[16];
    const char **rspCapability;
    char **euiccCiPKIdListForVerification;
    char **euiccCiPKIdListForSigning;
    const char *euiccCategory;
    const char **forbiddenProfilePolicyRules;
    char ppVersion[16];
    char sasAcreditationNumber[65];
    struct
    {
        char *platformLabel;
        char *discoveryBaseURL;
    } certificationDataObject;
};

int es10c_ex_get_euiccinfo2(struct euicc_ctx *ctx, struct es10c_ex_euiccinfo2 **euiccinfo2);
void es10c_ex_euiccinfo2_free(struct es10c_ex_euiccinfo2 *euiccinfo2);
