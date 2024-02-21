#include "info.h"
#include "cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10a.h>
#include <euicc/es10c.h>
#include <euicc/es10c_ex.h>

static int applet_main(int argc, char **argv)
{
    char *eid = NULL;
    struct es10a_euicc_configured_addresses addresses;
    struct es10c_ex_euiccinfo2 euiccinfo2;
    cJSON *jaddresses = NULL;
    cJSON *jeuiccinfo2 = NULL;
    cJSON *jdata = NULL;

    if (es10c_get_eid(&euicc_ctx, &eid))
    {
        jprint_error("es10c_get_eid", NULL);
        return -1;
    }

    if (es10a_get_euicc_configured_addresses(&euicc_ctx, &addresses) == 0)
    {
        jaddresses = cJSON_CreateObject();
    }

    if (es10c_ex_get_euiccinfo2(&euicc_ctx, &euiccinfo2) == 0)
    {
        jeuiccinfo2 = cJSON_CreateObject();
    }

    jdata = cJSON_CreateObject();
    cJSON_AddStringOrNullToObject(jdata, "eidValue", eid);
    free(eid);

    if (jaddresses)
    {
        cJSON_AddStringOrNullToObject(jaddresses, "defaultDpAddress", addresses.defaultDpAddress);
        cJSON_AddStringOrNullToObject(jaddresses, "rootDsAddress", addresses.rootDsAddress);
    }
    cJSON_AddItemToObject(jdata, "EuiccConfiguredAddresses", jaddresses);
    es10a_euicc_configured_addresses_free(&addresses);

    if (jeuiccinfo2)
    {
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "profileVersion", euiccinfo2.profileVersion);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "svn", euiccinfo2.svn);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "euiccFirmwareVer", euiccinfo2.euiccFirmwareVer);
        {
            cJSON *jextCardResource = cJSON_CreateObject();

            cJSON_AddNumberToObject(jextCardResource, "installedApplication", euiccinfo2.extCardResource.installedApplication);
            cJSON_AddNumberToObject(jextCardResource, "freeNonVolatileMemory", euiccinfo2.extCardResource.freeNonVolatileMemory);
            cJSON_AddNumberToObject(jextCardResource, "freeVolatileMemory", euiccinfo2.extCardResource.freeVolatileMemory);

            cJSON_AddItemToObject(jeuiccinfo2, "extCardResource", jextCardResource);
        }
        if (euiccinfo2.uiccCapability)
        {
            cJSON *juiccCapability = cJSON_CreateArray();
            for (int i = 0; euiccinfo2.uiccCapability[i] != NULL; i++)
            {
                cJSON_AddItemToArray(juiccCapability, cJSON_CreateString(euiccinfo2.uiccCapability[i]));
            }
            cJSON_AddItemToObject(jeuiccinfo2, "uiccCapability", juiccCapability);
        }
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "javacardVersion", euiccinfo2.javacardVersion);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "globalplatformVersion", euiccinfo2.globalplatformVersion);
        if (euiccinfo2.rspCapability)
        {
            cJSON *jrspCapability = cJSON_CreateArray();
            for (int i = 0; euiccinfo2.rspCapability[i] != NULL; i++)
            {
                cJSON_AddItemToArray(jrspCapability, cJSON_CreateString(euiccinfo2.rspCapability[i]));
            }
            cJSON_AddItemToObject(jeuiccinfo2, "rspCapability", jrspCapability);
        }
        if (euiccinfo2.euiccCiPKIdListForVerification)
        {
            cJSON *verification_keys = cJSON_CreateArray();
            for (int i = 0; euiccinfo2.euiccCiPKIdListForVerification[i] != NULL; i++)
            {
                cJSON_AddItemToArray(verification_keys, cJSON_CreateString(euiccinfo2.euiccCiPKIdListForVerification[i]));
            }
            cJSON_AddItemToObject(jeuiccinfo2, "euiccCiPKIdListForVerification", verification_keys);
        }
        if (euiccinfo2.euiccCiPKIdListForSigning)
        {
            cJSON *signing_keys = cJSON_CreateArray();
            for (int i = 0; euiccinfo2.euiccCiPKIdListForSigning[i] != NULL; i++)
            {
                cJSON_AddItemToArray(signing_keys, cJSON_CreateString(euiccinfo2.euiccCiPKIdListForSigning[i]));
            }
            cJSON_AddItemToObject(jeuiccinfo2, "euiccCiPKIdListForSigning", signing_keys);
        }
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "euiccCategory", euiccinfo2.euiccCategory);
        if (euiccinfo2.forbiddenProfilePolicyRules)
        {
            cJSON *jforbiddenProfilePolicyRules = cJSON_CreateArray();
            for (int i = 0; euiccinfo2.forbiddenProfilePolicyRules[i] != NULL; i++)
            {
                cJSON_AddItemToArray(jforbiddenProfilePolicyRules, cJSON_CreateString(euiccinfo2.forbiddenProfilePolicyRules[i]));
            }
            cJSON_AddItemToObject(jeuiccinfo2, "forbiddenProfilePolicyRules", jforbiddenProfilePolicyRules);
        }
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "ppVersion", euiccinfo2.ppVersion);
        cJSON_AddStringOrNullToObject(jeuiccinfo2, "sasAcreditationNumber", euiccinfo2.sasAcreditationNumber);
        {
            cJSON *jcertificationDataObject = cJSON_CreateObject();

            cJSON_AddStringOrNullToObject(jcertificationDataObject, "platformLabel", euiccinfo2.certificationDataObject.platformLabel);
            cJSON_AddStringOrNullToObject(jcertificationDataObject, "discoveryBaseURL", euiccinfo2.certificationDataObject.discoveryBaseURL);

            cJSON_AddItemToObject(jeuiccinfo2, "certificationDataObject", jcertificationDataObject);
        }
        es10c_ex_euiccinfo2_free(&euiccinfo2);
    }
    cJSON_AddItemToObject(jdata, "EUICCInfo2", jeuiccinfo2);

    jprint_success(jdata);

    return 0;
}

struct applet_entry applet_chip_info = {
    .name = "info",
    .main = applet_main,
};
