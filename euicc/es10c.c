#include "es10c.h"
#include "es10x.private.h"

#include "hexutil.h"
#include "base64.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "asn1c/asn1/ProfileInfoListRequest.h"
#include "asn1c/asn1/ProfileInfoListResponse.h"
#include "asn1c/asn1/EnableProfileRequest.h"
#include "asn1c/asn1/EnableProfileResponse.h"
#include "asn1c/asn1/DisableProfileRequest.h"
#include "asn1c/asn1/DisableProfileResponse.h"
#include "asn1c/asn1/DeleteProfileRequest.h"
#include "asn1c/asn1/DeleteProfileResponse.h"
#include "asn1c/asn1/EuiccMemoryResetRequest.h"
#include "asn1c/asn1/EuiccMemoryResetResponse.h"
#include "asn1c/asn1/GetEuiccDataRequest.h"
#include "asn1c/asn1/GetEuiccDataResponse.h"
#include "asn1c/asn1/SetNicknameRequest.h"
#include "asn1c/asn1/SetNicknameResponse.h"

int es10c_get_profiles_info(struct euicc_ctx *ctx, struct es10c_profile_info **profiles, int *profiles_count)
{
    int fret = 0;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    asn_dec_rval_t asn1drval;
    ProfileInfoListRequest_t *asn1req = NULL;
    ProfileInfoListResponse_t *asn1resp = NULL;

    *profiles = NULL;
    *profiles_count = 0;

    asn1req = malloc(sizeof(ProfileInfoListRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1erval = der_encode_to_buffer(&asn_DEF_ProfileInfoListRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_ProfileInfoListRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->g_asn1_der_request_buf, asn1erval.encoded) < 0)
    {
        goto err;
    }

    asn1drval = ber_decode(NULL, &asn_DEF_ProfileInfoListResponse, (void **)&asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    if (asn1resp->present != ProfileInfoListResponse_PR_profileInfoListOk)
    {
        goto err;
    }

    *profiles_count = asn1resp->choice.profileInfoListOk.list.count;
    *profiles = malloc(sizeof(struct es10c_profile_info) * (*profiles_count));
    if (!(*profiles))
    {
        goto err;
    }
    memset(*profiles, 0, sizeof(struct es10c_profile_info) * (*profiles_count));

    for (int i = 0; i < *profiles_count; i++)
    {
        struct es10c_profile_info *p = &((*profiles)[i]);
        struct ProfileInfo *asn1p = asn1resp->choice.profileInfoListOk.list.array[i];

        if (asn1p->iccid)
        {
            if (euicc_hexutil_bin2gsmbcd(p->iccid, sizeof(p->iccid), asn1p->iccid->buf, asn1p->iccid->size))
            {
                memset(p->iccid, 0, sizeof(p->iccid));
                continue;
            }
        }

        if (asn1p->isdpAid)
        {
            for (int j = 0; j < asn1p->isdpAid->size; j++)
            {
                sprintf(p->isdpAid + (j * 2), "%02X", (uint8_t)(asn1p->isdpAid->buf[j]));
            }
        }

        if (asn1p->profileState)
        {
            long profileState;

            asn_INTEGER2long(asn1p->profileState, &profileState);

            switch (profileState)
            {
            case ES10C_PROFILE_INFO_STATE_DISABLED:
                p->profileState = strdup("disabled");
                break;
            case ES10C_PROFILE_INFO_STATE_ENABLED:
                p->profileState = strdup("enabled");
                break;
            default:
                p->profileState = strdup("unknown");
                break;
            }
        }

        if (asn1p->profileClass)
        {
            long profileClass;

            asn_INTEGER2long(asn1p->profileClass, &profileClass);

            switch (profileClass)
            {
            case ES10C_PROFILE_INFO_CLASS_TEST:
                p->profileClass = strdup("test");
                break;
            case ES10C_PROFILE_INFO_CLASS_PROVISIONING:
                p->profileClass = strdup("provisioning");
                break;
            case ES10C_PROFILE_INFO_CLASS_OPERATIONAL:
                p->profileClass = strdup("operational");
                break;
            default:
                p->profileClass = strdup("unknown");
                break;
            }
        }

        if (asn1p->profileNickname)
        {
            p->profileNickname = malloc(asn1p->profileNickname->size + 1);
            if (p->profileNickname)
            {
                memcpy(p->profileNickname, asn1p->profileNickname->buf, asn1p->profileNickname->size);
                p->profileNickname[asn1p->profileNickname->size] = '\0';
            }
        }

        if (asn1p->serviceProviderName)
        {
            p->serviceProviderName = malloc(asn1p->serviceProviderName->size + 1);
            if (p->serviceProviderName)
            {
                memcpy(p->serviceProviderName, asn1p->serviceProviderName->buf, asn1p->serviceProviderName->size);
                p->serviceProviderName[asn1p->serviceProviderName->size] = '\0';
            }
        }

        if (asn1p->profileName)
        {
            p->profileName = malloc(asn1p->profileName->size + 1);
            if (p->profileName)
            {
                memcpy(p->profileName, asn1p->profileName->buf, asn1p->profileName->size);
                p->profileName[asn1p->profileName->size] = '\0';
            }
        }

        if (asn1p->iconType)
        {
            long iconType;

            asn_INTEGER2long(asn1p->iconType, &iconType);

            switch (iconType)
            {
            case ES10C_ICON_TYPE_JPEG:
                p->iconType = strdup("jpeg");
                break;
            case ES10C_ICON_TYPE_PNG:
                p->iconType = strdup("png");
                break;
            default:
                p->iconType = strdup("unknown");
                break;
            }
        }
        else
        {
            p->iconType = strdup("none");
        }

        if (asn1p->icon)
        {
            p->icon = malloc(euicc_base64_encode_len(asn1p->icon->size));
            if (p->icon)
            {
                euicc_base64_encode(p->icon, asn1p->icon->buf, asn1p->icon->size);
            }
        }
    }

    goto exit;

err:
    fret = -1;
    free(*profiles);
    *profiles = NULL;
    *profiles_count = 0;
exit:
    free(respbuf);
    ASN_STRUCT_FREE(asn_DEF_ProfileInfoListRequest, asn1req);
    ASN_STRUCT_FREE(asn_DEF_ProfileInfoListResponse, asn1resp);

    return fret;
}

static int iter_EnableProfileResponse(struct apdu_response *response, void *userdata)
{
    long *eresult = (long *)userdata;
    int fret = 0;
    asn_dec_rval_t asn1drval;
    EnableProfileResponse_t *asn1resp = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_EnableProfileResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    asn_INTEGER2long(&asn1resp->enableResult, eresult);
    goto exit;

err:
    fret = -1;
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_EnableProfileResponse, asn1resp);
    }
    return fret;
}

int es10c_enable_profile_aid(struct euicc_ctx *ctx, const char *aid, int refreshflag)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    EnableProfileRequest_t *asn1req = NULL;
    uint8_t asn1aid[16];
    unsigned long eresult;

    ret = euicc_hexutil_hex2bin(asn1aid, sizeof(asn1aid), aid);
    if (ret < 0)
    {
        goto err;
    }

    asn1req = malloc(sizeof(EnableProfileRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->refreshFlag = (refreshflag == 0) ? 0 : 1;

    asn1req->profileIdentifier.present = EnableProfileRequest__profileIdentifier_PR_isdpAid;
    ret = OCTET_STRING_fromBuf(&asn1req->profileIdentifier.choice.isdpAid, (const char *)asn1aid, ret);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_EnableProfileRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_EnableProfileRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_EnableProfileResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_EnableProfileRequest, asn1req);
    }
    return fret;
}

int es10c_enable_profile_iccid(struct euicc_ctx *ctx, const char *iccid, int refreshflag)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    EnableProfileRequest_t *asn1req = NULL;
    uint8_t asn1iccid[20];
    unsigned long eresult;

    ret = euicc_hexutil_gsmbcd2bin(asn1iccid, sizeof(asn1iccid), iccid);
    if (ret < 0)
    {
        goto err;
    }

    asn1req = malloc(sizeof(EnableProfileRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->refreshFlag = (refreshflag == 0) ? 0 : 1;

    asn1req->profileIdentifier.present = EnableProfileRequest__profileIdentifier_PR_iccid;
    ret = OCTET_STRING_fromBuf(&asn1req->profileIdentifier.choice.iccid, (const char *)asn1iccid, ret);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_EnableProfileRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_EnableProfileRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_EnableProfileResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_EnableProfileRequest, asn1req);
    }
    return fret;
}

static int iter_DisableProfileResponse(struct apdu_response *response, void *userdata)
{
    long *eresult = (long *)userdata;
    int fret = 0;
    asn_dec_rval_t asn1drval;
    DisableProfileResponse_t *asn1resp = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_DisableProfileResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    asn_INTEGER2long(&asn1resp->disableResult, eresult);
    goto exit;

err:
    fret = -1;
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_DisableProfileResponse, asn1resp);
    }
    return fret;
}

int es10c_disable_profile_aid(struct euicc_ctx *ctx, const char *aid, int refreshflag)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    DisableProfileRequest_t *asn1req = NULL;
    uint8_t asn1aid[16];
    unsigned long eresult;

    ret = euicc_hexutil_hex2bin(asn1aid, sizeof(asn1aid), aid);
    if (ret < 0)
    {
        goto err;
    }

    asn1req = malloc(sizeof(DisableProfileRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->refreshFlag = (refreshflag == 0) ? 0 : 1;

    asn1req->profileIdentifier.present = DisableProfileRequest__profileIdentifier_PR_isdpAid;
    ret = OCTET_STRING_fromBuf(&asn1req->profileIdentifier.choice.isdpAid, (const char *)asn1aid, ret);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_DisableProfileRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_DisableProfileRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_DisableProfileResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_DisableProfileRequest, asn1req);
    }
    return fret;
}

int es10c_disable_profile_iccid(struct euicc_ctx *ctx, const char *iccid, int refreshflag)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    DisableProfileRequest_t *asn1req = NULL;
    uint8_t asn1iccid[20];
    unsigned long eresult;

    ret = euicc_hexutil_gsmbcd2bin(asn1iccid, sizeof(asn1iccid), iccid);
    if (ret < 0)
    {
        goto err;
    }

    asn1req = malloc(sizeof(DisableProfileRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->refreshFlag = (refreshflag == 0) ? 0 : 1;

    asn1req->profileIdentifier.present = DisableProfileRequest__profileIdentifier_PR_iccid;
    ret = OCTET_STRING_fromBuf(&asn1req->profileIdentifier.choice.iccid, (const char *)asn1iccid, ret);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_DisableProfileRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_DisableProfileRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_DisableProfileResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_DisableProfileRequest, asn1req);
    }
    return fret;
}

static int iter_DeleteProfileResponse(struct apdu_response *response, void *userdata)
{
    long *eresult = (long *)userdata;
    int fret = 0;
    asn_dec_rval_t asn1drval;
    DeleteProfileResponse_t *asn1resp = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_DeleteProfileResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    asn_INTEGER2long(&asn1resp->deleteResult, eresult);
    goto exit;

err:
    fret = -1;
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_DeleteProfileResponse, asn1resp);
    }
    return fret;
}

int es10c_delete_profile_aid(struct euicc_ctx *ctx, const char *aid)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    DeleteProfileRequest_t *asn1req = NULL;
    uint8_t asn1aid[16];
    unsigned long eresult;

    ret = euicc_hexutil_hex2bin(asn1aid, sizeof(asn1aid), aid);
    if (ret < 0)
    {
        goto err;
    }

    asn1req = malloc(sizeof(DeleteProfileRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->present = DeleteProfileRequest_PR_isdpAid;
    ret = OCTET_STRING_fromBuf(&asn1req->choice.isdpAid, (const char *)asn1aid, ret);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_DeleteProfileRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_DeleteProfileRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_DeleteProfileResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_DeleteProfileRequest, asn1req);
    }
    return fret;
}

int es10c_delete_profile_iccid(struct euicc_ctx *ctx, const char *iccid)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    DeleteProfileRequest_t *asn1req = NULL;
    uint8_t asn1iccid[20];
    unsigned long eresult;

    ret = euicc_hexutil_gsmbcd2bin(asn1iccid, sizeof(asn1iccid), iccid);
    if (ret < 0)
    {
        goto err;
    }

    asn1req = malloc(sizeof(DeleteProfileRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->present = DeleteProfileRequest_PR_iccid;
    ret = OCTET_STRING_fromBuf(&asn1req->choice.iccid, (const char *)asn1iccid, ret);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_DeleteProfileRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_DeleteProfileRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_DeleteProfileResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_DeleteProfileRequest, asn1req);
    }
    return fret;
}

static int iter_EuiccMemoryResetResponse(struct apdu_response *response, void *userdata)
{
    long *eresult = (long *)userdata;
    int fret = 0;
    asn_dec_rval_t asn1drval;
    EuiccMemoryResetResponse_t *asn1resp = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_EuiccMemoryResetResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    asn_INTEGER2long(&asn1resp->resetResult, eresult);
    goto exit;

err:
    fret = -1;
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_EuiccMemoryResetResponse, asn1resp);
    }
    return fret;
}

int es10c_euicc_memory_reset(struct euicc_ctx *ctx, int op, int tp, int addr)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    EuiccMemoryResetRequest_t *asn1req = NULL;
    unsigned long eresult;

    asn1req = malloc(sizeof(EuiccMemoryResetRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->resetOptions.bits_unused = 5;
    asn1req->resetOptions.size = 1;
    asn1req->resetOptions.buf = malloc(asn1req->resetOptions.size);
    if (!asn1req->resetOptions.buf)
    {
        goto err;
    }

    asn1req->resetOptions.buf[0] = 0;

    if (op)
    {
        asn1req->resetOptions.buf[0] |= 1 << (7 - EuiccMemoryResetRequest__resetOptions_deleteOperationalProfiles);
    }

    if (tp)
    {
        asn1req->resetOptions.buf[0] |= 1 << (7 - EuiccMemoryResetRequest__resetOptions_deleteFieldLoadedTestProfiles);
    }

    if (addr)
    {
        asn1req->resetOptions.buf[0] |= 1 << (7 - EuiccMemoryResetRequest__resetOptions_resetDefaultSmdpAddress);
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_EuiccMemoryResetRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_EuiccMemoryResetRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_EuiccMemoryResetResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_EuiccMemoryResetRequest, asn1req);
    }
    return fret;
}

static int iter_GetEuiccDataResponse(struct apdu_response *response, void *userdata)
{
    int fret = 0;
    char **eid = (char **)userdata;
    asn_dec_rval_t asn1drval;
    GetEuiccDataResponse_t *asn1resp = NULL;

    *eid = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_GetEuiccDataResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    *eid = malloc((asn1resp->eidValue.size * 2) + 1);
    if (*eid == NULL)
    {
        goto err;
    }

    if (euicc_hexutil_bin2hex(*eid, (asn1resp->eidValue.size * 2) + 1, asn1resp->eidValue.buf, asn1resp->eidValue.size))
    {
        goto err;
    }

    goto exit;

err:
    fret = -1;
    free(*eid);
    *eid = NULL;
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_GetEuiccDataResponse, asn1resp);
    }

    return fret;
}

int es10c_get_eid(struct euicc_ctx *ctx, char **eid)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    GetEuiccDataRequest_t *asn1req = NULL;

    asn1req = malloc(sizeof(GetEuiccDataRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    ret = OCTET_STRING_fromBuf(&asn1req->tagList, "\x5A", 1);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_GetEuiccDataRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_GetEuiccDataRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_GetEuiccDataResponse, eid);
    if (ret < 0)
    {
        goto err;
    }

    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_GetEuiccDataRequest, asn1req);
    }

    return fret;
}

static int iter_SetNicknameResponse(struct apdu_response *response, void *userdata)
{
    long *eresult = (long *)userdata;
    int fret = 0;
    asn_dec_rval_t asn1drval;
    SetNicknameResponse_t *asn1resp = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_SetNicknameResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    asn_INTEGER2long(&asn1resp->setNicknameResult, eresult);
    goto exit;

err:
    fret = -1;
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_SetNicknameResponse, asn1resp);
    }

    return fret;
}

int es10c_set_nickname(struct euicc_ctx *ctx, const char *iccid, const char *nickname)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    SetNicknameRequest_t *asn1req = NULL;
    uint8_t asn1iccid[20];
    unsigned long eresult;

    asn1req = malloc(sizeof(SetNicknameRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    ret = euicc_hexutil_gsmbcd2bin(asn1iccid, sizeof(asn1iccid), iccid);
    if (ret < 0)
    {
        goto err;
    }

    ret = OCTET_STRING_fromBuf(&asn1req->iccid, (const char *)asn1iccid, ret);
    if (ret < 0)
    {
        goto err;
    }

    ret = OCTET_STRING_fromBuf(&asn1req->profileNickname, nickname, strlen(nickname));
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_SetNicknameRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_SetNicknameRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_SetNicknameResponse, &eresult);
    if (ret < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    if (asn1req)
    {
        ASN_STRUCT_FREE(asn_DEF_SetNicknameRequest, asn1req);
    }

    return fret;
}

void es10c_profile_info_free_all(struct es10c_profile_info *profiles, int count)
{
    if (!profiles)
    {
        return;
    }
    for (int i = 0; i < count; i++)
    {
        free(profiles[i].profileState);
        free(profiles[i].profileClass);
        free(profiles[i].profileNickname);
        free(profiles[i].serviceProviderName);
        free(profiles[i].profileName);
        free(profiles[i].iconType);
        free(profiles[i].icon);
    }
    free(profiles);
}

void es10c_profile_info_print(struct es10c_profile_info *p)
{
    printf("\ticcid: %s\n", p->iccid);
    printf("\tisdpAid: %s\n", p->isdpAid);
    printf("\tprofileState: %s\n", p->profileState);
    printf("\tprofileNickname: %s\n", p->profileNickname ? p->profileNickname : "(null)");
    printf("\tserviceProviderName: %s\n", p->serviceProviderName ? p->serviceProviderName : "(null)");
    printf("\tprofileName: %s\n", p->profileName ? p->profileName : "(null)");
    printf("\tprofileClass: %s\n", p->profileClass);
}
