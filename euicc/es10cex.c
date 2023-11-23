#include "es10cex.h"
#include "es10x.private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asn1c/asn1/GetEuiccInfo2Request.h"
#include "asn1c/asn1/EUICCInfo2.h"

static int _versiontype_to_string(char *out, int out_len, VersionType_t version)
{
    if (version.size != 3)
        return -1;

    return snprintf(out, out_len, "%d.%d.%d", version.buf[0], version.buf[1], version.buf[2]);
}

int es10cex_get_euiccinfo2(struct euicc_ctx *ctx, struct es10cex_euiccinfo2 *info)
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

    asn1drval = ber_decode(NULL, &asn_DEF_EUICCInfo2, (void **)&asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    memset(info, 0, sizeof(*info));

    _versiontype_to_string(info->profile_version, sizeof(info->profile_version), asn1resp->profileVersion);
    _versiontype_to_string(info->sgp22_version, sizeof(info->sgp22_version), asn1resp->svn);
    _versiontype_to_string(info->euicc_firmware_version, sizeof(info->euicc_firmware_version), asn1resp->euiccFirmwareVer);
    _versiontype_to_string(info->pp_version, sizeof(info->pp_version), asn1resp->ppVersion);
    if (asn1resp->javacardVersion)
    {
        _versiontype_to_string(info->uicc_firmware_version, sizeof(info->uicc_firmware_version),
                           *asn1resp->javacardVersion);
    }
    if (asn1resp->globalplatformVersion)
    {
        _versiontype_to_string(info->global_platform_version, sizeof(info->global_platform_version),
                           *asn1resp->globalplatformVersion);
    }
    memcpy(info->sas_accreditation_number, asn1resp->sasAcreditationNumber.buf,
           asn1resp->sasAcreditationNumber.size);

    for (int i = 0; i < asn1resp->extCardResource.size;)
    {
        uint8_t tag = asn1resp->extCardResource.buf[i];
        i++;
        uint8_t length = asn1resp->extCardResource.buf[i];
        i++;
        uint8_t *b = &asn1resp->extCardResource.buf[i];
        switch (tag)
        {
        case 0x81:
            info->installed_app = b[0];
            i += length;
        case 0x82:
            info->free_nvram = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | (b[0]);
            i += length;
        case 0x83:
            info->free_ram = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | (b[0]);
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
