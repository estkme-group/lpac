#include "euicc.private.h"
#include "es10c_ex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "derutil.h"

#include "asn1c/asn1/GetEuiccInfo2Request.h"
#include "asn1c/asn1/EUICCInfo2.h"
#include "euicc/hexutil.h"

static int _versiontype_to_string(char *out, int out_len, VersionType_t version)
{
    if (version.size != 3)
        return -1;

    return snprintf(out, out_len, "%d.%d.%d", version.buf[0], version.buf[1], version.buf[2]);
}

static int _read_ext_resource(uint8_t *buf, int len, uint16_t tag)
{
    int ret;
    uint8_t *value;
    int value_len;

    value_len = euicc_derutil_tag_find(&value, buf, len, &tag, 0);
    if (value_len == -1)
    {
        return -1;
    }

    ret = 0;
    for (int i = 0; i < value_len; i++)
    {
        ret |= (value[i] << (8 * (value_len - i - 1)));
    }

    return ret;
}

static int _read_bitwise_cap(const char ***output, uint8_t *flags, int flags_len, const char **capdesc)
{
    int max_cap_len = 0;
    int flags_reg;
    int flags_count;
    const char **wptr;

    *output = NULL;

    for (max_cap_len = 0; capdesc[max_cap_len]; max_cap_len++)
        ;

    for (int j = 0; j < flags_len; j++)
    {
        flags_reg = flags[j];
        for (int i = 0; (i < 8) && ((j * 8 + i) < max_cap_len); i++)
        {
            if (flags_reg & (0b10000000))
            {
                flags_count++;
            }
            flags_reg <<= 1;
        }
    }

    wptr = calloc(flags_count + 1, sizeof(char *));
    if (!wptr)
    {
        return -1;
    }
    *output = wptr;

    for (int j = 0; j < flags_len; j++)
    {
        flags_reg = flags[j];

        for (int i = 0; (i < 8) && ((j * 8 + i) < max_cap_len); i++)
        {
            if (flags_reg & 0b10000000)
            {
                *(wptr++) = capdesc[j * 8 + i];
            }
            flags_reg <<= 1;
        }
    }

    return 0;
}

int es10c_ex_get_euiccinfo2(struct euicc_ctx *ctx, struct es10c_ex_euiccinfo2 **euiccinfo2)
{
    int fret = 0;
    struct es10c_ex_euiccinfo2 *info;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    asn_dec_rval_t asn1drval;
    GetEuiccInfo2Request_t *asn1req = NULL;
    EUICCInfo2_t *asn1resp = NULL;

    *euiccinfo2 = calloc(1, sizeof(struct es10c_ex_euiccinfo2));
    if (*euiccinfo2 == NULL)
    {
        goto err;
    }
    info = *euiccinfo2;

    asn1req = malloc(sizeof(GetEuiccInfo2Request_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1erval = der_encode_to_buffer(&asn_DEF_GetEuiccInfo2Request, asn1req, ctx->apdu_request_buffer.body,
                                     sizeof(ctx->apdu_request_buffer.body));
    ASN_STRUCT_FREE(asn_DEF_GetEuiccInfo2Request, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu_request_buffer.body, asn1erval.encoded) < 0)
    {
        goto err;
    }

    asn1drval = ber_decode(NULL, &asn_DEF_EUICCInfo2, (void **)&asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    memset(info, 0, sizeof(*info));

    _versiontype_to_string(info->profileVersion, sizeof(info->profileVersion), asn1resp->profileVersion);

    _versiontype_to_string(info->svn, sizeof(info->svn), asn1resp->svn);

    _versiontype_to_string(info->euiccFirmwareVer, sizeof(info->euiccFirmwareVer), asn1resp->euiccFirmwareVer);

    info->extCardResource.installedApplication = _read_ext_resource(asn1resp->extCardResource.buf, asn1resp->extCardResource.size, 0x81);
    info->extCardResource.freeNonVolatileMemory = _read_ext_resource(asn1resp->extCardResource.buf, asn1resp->extCardResource.size, 0x82);
    info->extCardResource.freeVolatileMemory = _read_ext_resource(asn1resp->extCardResource.buf, asn1resp->extCardResource.size, 0x83);

    if (asn1resp->uiccCapability.size > 0)
    {
        static const char *desc[] = {"contactlessSupport", "usimSupport", "isimSupport", "csimSupport", "akaMilenage", "akaCave", "akaTuak128", "akaTuak256", "rfu1", "rfu2", "gbaAuthenUsim", "gbaAuthenISim", "mbmsAuthenUsim", "eapClient", "javacard", "multos", "multipleUsimSupport", "multipleIsimSupport", "multipleCsimSupport"};

        if (_read_bitwise_cap(&info->uiccCapability, asn1resp->uiccCapability.buf, asn1resp->uiccCapability.size, desc))
        {
            goto err;
        }
    }

    if (asn1resp->javacardVersion)
    {
        _versiontype_to_string(info->javacardVersion, sizeof(info->javacardVersion),
                               *asn1resp->javacardVersion);
    }

    if (asn1resp->globalplatformVersion)
    {
        _versiontype_to_string(info->globalplatformVersion, sizeof(info->globalplatformVersion),
                               *asn1resp->globalplatformVersion);
    }

    if (asn1resp->rspCapability.size > 0)
    {
        static const char *desc[] = {"additionalProfile", "crlSupport", "rpmSupport", "testProfileSupport"};

        if (_read_bitwise_cap(&info->rspCapability, asn1resp->rspCapability.buf, asn1resp->rspCapability.size, desc))
        {
            goto err;
        }
    }

    if (asn1resp->euiccCiPKIdListForVerification.list.count)
    {
        info->euiccCiPKIdListForVerification = calloc(asn1resp->euiccCiPKIdListForVerification.list.count + 1, sizeof(char *));
        if (!info->euiccCiPKIdListForVerification)
        {
            goto err;
        }

        for (int i = 0; i < asn1resp->euiccCiPKIdListForVerification.list.count; i++)
        {
            info->euiccCiPKIdListForVerification[i] = malloc((asn1resp->euiccCiPKIdListForVerification.list.array[i]->size * 2 + 1) * sizeof(char));
            if (!info->euiccCiPKIdListForVerification[i])
            {
                goto err;
            }

            euicc_hexutil_bin2hex(info->euiccCiPKIdListForVerification[i], asn1resp->euiccCiPKIdListForVerification.list.array[i]->size * 2 + 1,
                                  asn1resp->euiccCiPKIdListForVerification.list.array[i]->buf, asn1resp->euiccCiPKIdListForVerification.list.array[i]->size);
        }

        info->euiccCiPKIdListForVerification[asn1resp->euiccCiPKIdListForVerification.list.count] = NULL;
    }

    if (asn1resp->euiccCiPKIdListForSigning.list.count)
    {
        info->euiccCiPKIdListForSigning = calloc(asn1resp->euiccCiPKIdListForSigning.list.count + 1, sizeof(char *));
        if (!info->euiccCiPKIdListForSigning)
        {
            goto err;
        }

        for (int i = 0; i < asn1resp->euiccCiPKIdListForSigning.list.count; i++)
        {
            info->euiccCiPKIdListForSigning[i] = malloc((asn1resp->euiccCiPKIdListForSigning.list.array[i]->size * 2 + 1) * sizeof(char));
            if (!info->euiccCiPKIdListForSigning[i])
            {
                goto err;
            }

            euicc_hexutil_bin2hex(info->euiccCiPKIdListForSigning[i], asn1resp->euiccCiPKIdListForSigning.list.array[i]->size * 2 + 1,
                                  asn1resp->euiccCiPKIdListForSigning.list.array[i]->buf, asn1resp->euiccCiPKIdListForSigning.list.array[i]->size);
        }

        info->euiccCiPKIdListForSigning[asn1resp->euiccCiPKIdListForSigning.list.count] = NULL;
    }

    if (asn1resp->euiccCategory)
    {
        long tmplong = 0;

        asn_INTEGER2long(asn1resp->euiccCategory, &tmplong);

        switch (tmplong)
        {
        case 1:
            info->euiccCategory = "basicEuicc";
            break;
        case 2:
            info->euiccCategory = "mediumEuicc";
            break;
        case 3:
            info->euiccCategory = "contactlessEuicc";
            break;
        case 0:
        default:
            info->euiccCategory = "other";
            break;
        }
    }

    if (asn1resp->forbiddenProfilePolicyRules)
    {
        static const char *desc[] = {"pprUpdateControl", "ppr1", "ppr2", "ppr3"};

        if (_read_bitwise_cap(&info->forbiddenProfilePolicyRules, asn1resp->forbiddenProfilePolicyRules->buf, asn1resp->forbiddenProfilePolicyRules->size, desc))
        {
            goto err;
        }
    }

    _versiontype_to_string(info->ppVersion, sizeof(info->ppVersion), asn1resp->ppVersion);

    memcpy(info->sasAcreditationNumber, asn1resp->sasAcreditationNumber.buf,
           asn1resp->sasAcreditationNumber.size);

    if (asn1resp->certificationDataObject)
    {
        info->certificationDataObject.platformLabel = calloc(asn1resp->certificationDataObject->platformLabel.size + 1, sizeof(char));
        if (!info->certificationDataObject.platformLabel)
        {
            goto err;
        }
        memcpy(info->certificationDataObject.platformLabel, asn1resp->certificationDataObject->platformLabel.buf, asn1resp->certificationDataObject->platformLabel.size);

        info->certificationDataObject.discoveryBaseURL = calloc(asn1resp->certificationDataObject->discoveryBaseURL.size + 1, sizeof(char));
        if (!info->certificationDataObject.discoveryBaseURL)
        {
            goto err;
        }
        memcpy(info->certificationDataObject.discoveryBaseURL, asn1resp->certificationDataObject->discoveryBaseURL.buf, asn1resp->certificationDataObject->discoveryBaseURL.size);
    }

    goto exit;
err:
    fret = -1;
    free(*euiccinfo2);
exit:
    free(respbuf);
    ASN_STRUCT_FREE(asn_DEF_GetEuiccInfo2Request, asn1req);
    ASN_STRUCT_FREE(asn_DEF_EUICCInfo2, asn1resp);

    return fret;
}

void es10c_ex_euiccinfo2_free(struct es10c_ex_euiccinfo2 *euiccinfo2)
{
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

    free(euiccinfo2->uiccCapability);
    free(euiccinfo2->rspCapability);
    free(euiccinfo2->forbiddenProfilePolicyRules);
    free(euiccinfo2->certificationDataObject.discoveryBaseURL);
    free(euiccinfo2->certificationDataObject.platformLabel);

    free(euiccinfo2);
}
