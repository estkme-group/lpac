#include "es10cex.h"
#include "es10x.private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asn1c/asn1/GetEuiccInfo1Request.h"
#include "asn1c/asn1/GetEuiccInfo2Request.h"
#include "asn1c/asn1/EUICCInfo2.h"

static int es10cex_version_to_string(VersionType_t version, char **out);

int es10cex_get_euicc_info(struct euicc_ctx *ctx, struct es10cex_euicc_info *info)
{
    int fret = 0;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    asn_dec_rval_t asn1drval;
    GetEuiccInfo2Request_t *asn1req = NULL;
    EUICCInfo2_t *asn1resp = NULL;

    asn1req = malloc(sizeof(GetEuiccInfo2Request_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1erval = der_encode_to_buffer(&asn_DEF_GetEuiccInfo2Request, asn1req, ctx->g_asn1_der_request_buf,
                                     sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_GetEuiccInfo2Request, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->g_asn1_der_request_buf, asn1erval.encoded) < 0)
    {
        goto err;
    }

    asn1drval = ber_decode(NULL, &asn_DEF_EUICCInfo2, (void **) &asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    if (es10cex_version_to_string(asn1resp->profileVersion, &info->profile_version))
    {
        goto err;
    }

    if (es10cex_version_to_string(asn1resp->svn, &info->sgp22_version))
    {
        goto err;
    }

    if (es10cex_version_to_string(asn1resp->euiccFirmwareVer, &info->euicc_firmware_version))
    {
        goto err;
    }

    if (es10cex_version_to_string(asn1resp->ppVersion, &info->pp_version))
    {
        goto err;
    }

    if (asn1resp->javacardVersion)
    {
        if (es10cex_version_to_string(*asn1resp->javacardVersion, &info->uicc_firmware_version))
        {
            goto err;
        }
    }

    if (asn1resp->globalplatformVersion)
    {
        if (es10cex_version_to_string(*asn1resp->globalplatformVersion, &info->global_platform_version))
        {
            goto err;
        }
    }

    info->sas_accreditation_number = malloc(asn1resp->sasAcreditationNumber.size + 1);
    if (info->sas_accreditation_number)
    {
        memcpy(info->sas_accreditation_number, asn1resp->sasAcreditationNumber.buf,
               asn1resp->sasAcreditationNumber.size);
        info->sas_accreditation_number[asn1resp->sasAcreditationNumber.size + 1] = '\0';
    }

    for (int i = 0; i < asn1resp->extCardResource.size;) {
        uint8_t tag = asn1resp->extCardResource.buf[i];
        i++;
        uint8_t length = asn1resp->extCardResource.buf[i];
        i++;
        switch (tag) {
            case 0x81:
                info->installed_app = *((uint8_t *) &(asn1resp->extCardResource.buf[i]));
                i += length;
            case 0x82:
                info->free_nvram = *((uint32_t *) &(asn1resp->extCardResource.buf[i]));
                i += length;
            case 0x83:
                info->free_ram = *((uint32_t *) &(asn1resp->extCardResource.buf[i]));
                i += length;
            default:
                i += length;
                continue;
        }
    }

    goto exit;
err:
    fret = -1;
exit:
    free(respbuf);
    ASN_STRUCT_FREE(asn_DEF_GetEuiccInfo2Request, asn1req);
    ASN_STRUCT_FREE(asn_DEF_EUICCInfo2, asn1resp);

    return fret;
}

void es10cex_euicc_info_free(struct es10cex_euicc_info *info)
{
    if (!info) {
        return;
    }
    free(info->profile_version);
    free(info->sgp22_version);
    free(info->euicc_firmware_version);
    free(info->uicc_firmware_version);
    free(info->global_platform_version);
    free(info->sas_accreditation_number);
    free(info->pp_version);
}

static int es10cex_version_to_string(VersionType_t version, char **out)
{
    if (version.size != 3) return -1;
    char buf[12];
    int n = snprintf(buf, 12, "%d.%d.%d", version.buf[0], version.buf[1], version.buf[2]);
    *out = malloc(n + 1);
    if (*out)
    {
        strncpy(*out, buf, n);
        return 0;
    }
    return -1;
}
