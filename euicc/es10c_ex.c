#define _GNU_SOURCE
#include "euicc.private.h"
#include "es10c_ex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "derutil.h"
#include "hexutil.h"

static int _versiontype2str(char **out, const uint8_t *buffer, uint8_t buffer_len)
{
    if (buffer_len != 3)
    {
        return -1;
    }
    return asprintf(out, "%d.%d.%d", buffer[0], buffer[1], buffer[2]);
}

int es10c_ex_get_euiccinfo2(struct euicc_ctx *ctx, struct es10c_ex_euiccinfo2 *euiccinfo2)
{
    int fret = 0;
    struct euicc_derutil_node n_request = {
        .tag = 0xBF22, // GetEuiccInfo2Request
    };
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct euicc_derutil_node tmpnode, tmpchidnode, n_EUICCInfo2;

    memset(euiccinfo2, 0, sizeof(struct es10c_ex_euiccinfo2));

    reqlen = sizeof(ctx->apdu._internal.request_buffer.body);
    if (euicc_derutil_pack(ctx->apdu._internal.request_buffer.body, &reqlen, &n_request) < 0)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu._internal.request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&n_EUICCInfo2, n_request.tag, respbuf, resplen) < 0)
    {
        goto err;
    }

    tmpnode.self.ptr = n_EUICCInfo2.value;
    tmpnode.self.length = 0;
    while (euicc_derutil_unpack_next(&tmpnode, &tmpnode, n_EUICCInfo2.value, n_EUICCInfo2.length) == 0)
    {
        switch (tmpnode.tag)
        {
        case 0x81: // profileVersion
            _versiontype2str(&euiccinfo2->profileVersion, tmpnode.value, tmpnode.length);
            break;
        case 0x82: // svn
            _versiontype2str(&euiccinfo2->svn, tmpnode.value, tmpnode.length);
            break;
        case 0x83: // euiccFirmwareVer
            _versiontype2str(&euiccinfo2->euiccFirmwareVer, tmpnode.value, tmpnode.length);
            break;
        case 0x84: // extCardResource
            tmpchidnode.self.ptr = tmpnode.value;
            tmpchidnode.self.length = 0;
            while (euicc_derutil_unpack_next(&tmpchidnode, &tmpchidnode, tmpnode.value, tmpnode.length) == 0)
            {
                switch (tmpchidnode.tag)
                {
                case 0x81:
                    euiccinfo2->extCardResource.installedApplication = euicc_derutil_convert_bin2long(tmpchidnode.value, tmpchidnode.length);
                    break;
                case 0x82:
                    euiccinfo2->extCardResource.freeNonVolatileMemory = euicc_derutil_convert_bin2long(tmpchidnode.value, tmpchidnode.length);
                    break;
                case 0x83:
                    euiccinfo2->extCardResource.freeVolatileMemory = euicc_derutil_convert_bin2long(tmpchidnode.value, tmpchidnode.length);
                    break;
                }
            }
            break;
        case 0x85: // uiccCapability
        {
            static const char *desc[] = {"contactlessSupport", "usimSupport", "isimSupport", "csimSupport", "akaMilenage", "akaCave", "akaTuak128", "akaTuak256", "rfu1", "rfu2", "gbaAuthenUsim", "gbaAuthenISim", "mbmsAuthenUsim", "eapClient", "javacard", "multos", "multipleUsimSupport", "multipleIsimSupport", "multipleCsimSupport"};

            if (euicc_derutil_convert_bin2bits_str(&euiccinfo2->uiccCapability, tmpnode.value, tmpnode.length, desc))
            {
                goto err;
            }
        }
        break;
        case 0x86: // javacardVersion
            _versiontype2str(&euiccinfo2->javacardVersion, tmpnode.value, tmpnode.length);
            break;
        case 0x87: // globalplatformVersion
            _versiontype2str(&euiccinfo2->globalplatformVersion, tmpnode.value, tmpnode.length);
            break;
        case 0x88: // rspCapability
        {
            static const char *desc[] = {"additionalProfile", "crlSupport", "rpmSupport", "testProfileSupport"};

            if (euicc_derutil_convert_bin2bits_str(&euiccinfo2->rspCapability, tmpnode.value, tmpnode.length, desc))
            {
                goto err;
            }
        }
        break;
        case 0xA9: // euiccCiPKIdListForVerification
        {
            int count;

            tmpchidnode.self.ptr = tmpnode.value;
            tmpchidnode.self.length = 0;
            count = 0;
            while (euicc_derutil_unpack_next(&tmpchidnode, &tmpchidnode, tmpnode.value, tmpnode.length) == 0)
            {
                count++;
            }

            euiccinfo2->euiccCiPKIdListForVerification = malloc((count + 1) * sizeof(char *));
            if (!euiccinfo2->euiccCiPKIdListForVerification)
            {
                goto err;
            }
            memset(euiccinfo2->euiccCiPKIdListForVerification, 0, (count + 1) * sizeof(char *));

            tmpchidnode.self.ptr = tmpnode.value;
            tmpchidnode.self.length = 0;
            count = 0;
            while (euicc_derutil_unpack_next(&tmpchidnode, &tmpchidnode, tmpnode.value, tmpnode.length) == 0)
            {
                euiccinfo2->euiccCiPKIdListForVerification[count] = malloc((tmpchidnode.length * 2 + 1) * sizeof(char));
                if (!euiccinfo2->euiccCiPKIdListForVerification[count])
                {
                    goto err;
                }

                euicc_hexutil_bin2hex(euiccinfo2->euiccCiPKIdListForVerification[count], tmpchidnode.length * 2 + 1,
                                      tmpchidnode.value, tmpchidnode.length);
                count++;
            }
        }
        break;
        case 0xAA: // euiccCiPKIdListForSigning
        {
            int count;

            tmpchidnode.self.ptr = tmpnode.value;
            tmpchidnode.self.length = 0;
            count = 0;
            while (euicc_derutil_unpack_next(&tmpchidnode, &tmpchidnode, tmpnode.value, tmpnode.length) == 0)
            {
                count++;
            }

            euiccinfo2->euiccCiPKIdListForSigning = malloc((count + 1) * sizeof(char *));
            if (!euiccinfo2->euiccCiPKIdListForSigning)
            {
                goto err;
            }
            memset(euiccinfo2->euiccCiPKIdListForSigning, 0, (count + 1) * sizeof(char *));

            tmpchidnode.self.ptr = tmpnode.value;
            tmpchidnode.self.length = 0;
            count = 0;
            while (euicc_derutil_unpack_next(&tmpchidnode, &tmpchidnode, tmpnode.value, tmpnode.length) == 0)
            {
                euiccinfo2->euiccCiPKIdListForSigning[count] = malloc((tmpchidnode.length * 2 + 1) * sizeof(char));
                if (!euiccinfo2->euiccCiPKIdListForSigning[count])
                {
                    goto err;
                }

                euicc_hexutil_bin2hex(euiccinfo2->euiccCiPKIdListForSigning[count], tmpchidnode.length * 2 + 1,
                                      tmpchidnode.value, tmpchidnode.length);
                count++;
            }
        }
        break;
        case 0xAB: // euiccCategory
        {
            switch (euicc_derutil_convert_bin2long(tmpnode.value, tmpnode.length))
            {
            case 1:
                euiccinfo2->euiccCategory = "basicEuicc";
                break;
            case 2:
                euiccinfo2->euiccCategory = "mediumEuicc";
                break;
            case 3:
                euiccinfo2->euiccCategory = "contactlessEuicc";
                break;
            case 0:
            default:
                euiccinfo2->euiccCategory = "other";
                break;
            }
        }
        break;
        case 0x99: // forbiddenProfilePolicyRules
        {
            static const char *desc[] = {"pprUpdateControl", "ppr1", "ppr2", "ppr3"};

            if (euicc_derutil_convert_bin2bits_str(&euiccinfo2->forbiddenProfilePolicyRules, tmpnode.value, tmpnode.length, desc))
            {
                goto err;
            }
        }
        break;
        case 0x04: // ppVersion
            _versiontype2str(&euiccinfo2->ppVersion, tmpnode.value, tmpnode.length);
            break;
        case 0x0C: // sasAcreditationNumber
            euiccinfo2->sasAcreditationNumber = malloc(tmpnode.length + 1);
            if (!euiccinfo2->sasAcreditationNumber)
            {
                goto err;
            }
            memcpy(euiccinfo2->sasAcreditationNumber, tmpnode.value, tmpnode.length);
            euiccinfo2->sasAcreditationNumber[tmpnode.length] = 0;
            break;
        case 0xAC: // certificationDataObject
            tmpchidnode.self.ptr = tmpnode.value;
            tmpchidnode.self.length = 0;
            while (euicc_derutil_unpack_next(&tmpchidnode, &tmpchidnode, tmpnode.value, tmpnode.length) == 0)
            {
                switch (tmpchidnode.tag)
                {
                case 0x80:
                    euiccinfo2->certificationDataObject.platformLabel = malloc(tmpchidnode.length + 1);
                    if (!euiccinfo2->certificationDataObject.platformLabel)
                    {
                        goto err;
                    }
                    memcpy(euiccinfo2->certificationDataObject.platformLabel, tmpchidnode.value, tmpchidnode.length);
                    euiccinfo2->certificationDataObject.platformLabel[tmpchidnode.length] = 0;
                    break;
                case 0x81:
                    euiccinfo2->certificationDataObject.discoveryBaseURL = malloc(tmpchidnode.length + 1);
                    if (!euiccinfo2->certificationDataObject.discoveryBaseURL)
                    {
                        goto err;
                    }
                    memcpy(euiccinfo2->certificationDataObject.discoveryBaseURL, tmpchidnode.value, tmpchidnode.length);
                    euiccinfo2->certificationDataObject.discoveryBaseURL[tmpchidnode.length] = 0;
                    break;
                }
            }
            break;
        }
    }

    fret = 0;
    goto exit;

err:
    fret = -1;
    es10c_ex_euiccinfo2_free(euiccinfo2);
exit:
    free(respbuf);
    respbuf = NULL;
    return fret;
}

void es10c_ex_euiccinfo2_free(struct es10c_ex_euiccinfo2 *euiccinfo2)
{
    if (!euiccinfo2)
    {
        return;
    }

    free(euiccinfo2->profileVersion);
    free(euiccinfo2->svn);
    free(euiccinfo2->euiccFirmwareVer);
    free(euiccinfo2->uiccCapability);
    free(euiccinfo2->javacardVersion);
    free(euiccinfo2->globalplatformVersion);
    free(euiccinfo2->rspCapability);
    if (euiccinfo2->euiccCiPKIdListForVerification)
    {
        for (int i = 0; euiccinfo2->euiccCiPKIdListForVerification[i] != NULL; i++)
        {
            free(euiccinfo2->euiccCiPKIdListForVerification[i]);
        }
        free(euiccinfo2->euiccCiPKIdListForVerification);
    }
    if (euiccinfo2->euiccCiPKIdListForSigning)
    {
        for (int i = 0; euiccinfo2->euiccCiPKIdListForSigning[i] != NULL; i++)
        {
            free(euiccinfo2->euiccCiPKIdListForSigning[i]);
        }
        free(euiccinfo2->euiccCiPKIdListForSigning);
    }
    free(euiccinfo2->forbiddenProfilePolicyRules);
    free(euiccinfo2->ppVersion);
    free(euiccinfo2->sasAcreditationNumber);
    free(euiccinfo2->certificationDataObject.discoveryBaseURL);
    free(euiccinfo2->certificationDataObject.platformLabel);

    memset(euiccinfo2, 0, sizeof(struct es10c_ex_euiccinfo2));
}
