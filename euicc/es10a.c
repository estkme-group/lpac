#include "es10a.h"
#include "es10x.private.h"

#include "hexutil.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "asn1c/asn1/EuiccConfiguredAddressesRequest.h"
#include "asn1c/asn1/EuiccConfiguredAddressesResponse.h"
#include "asn1c/asn1/SetDefaultDpAddressRequest.h"
#include "asn1c/asn1/SetDefaultDpAddressResponse.h"

struct userdata_get_euicc_configured_addresses
{
    char **smds;
    char **smdp;
};

static int iter_EuiccConfiguredAddressesResponse(struct apdu_response *response, void *userdata)
{
    struct userdata_get_euicc_configured_addresses *ud = (struct userdata_get_euicc_configured_addresses *)userdata;
    int fret = 0;
    asn_dec_rval_t asn1drval;
    EuiccConfiguredAddressesResponse_t *asn1resp = NULL;

    if (ud->smds)
        *ud->smds = NULL;
    if (ud->smdp)
        *ud->smdp = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_EuiccConfiguredAddressesResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    if (ud->smds)
    {
        *ud->smds = malloc(asn1resp->rootDsAddress.size + 1);
        if (!*ud->smds)
        {
            goto err;
        }
        memcpy(*ud->smds, asn1resp->rootDsAddress.buf, asn1resp->rootDsAddress.size);
        (*ud->smds)[asn1resp->rootDsAddress.size] = '\0';
    }

    if (ud->smdp)
    {
        if (asn1resp->defaultDpAddress)
        {
            *ud->smdp = malloc(asn1resp->defaultDpAddress->size + 1);
            if (!*ud->smdp)
            {
                goto err;
            }
            memcpy(*ud->smdp, asn1resp->defaultDpAddress->buf, asn1resp->defaultDpAddress->size);
            (*ud->smdp)[asn1resp->defaultDpAddress->size] = '\0';
        }
        else
        {
            *ud->smdp = malloc(1);
            if (!*ud->smdp)
            {
                goto err;
            }
            (*ud->smdp)[0] = '\0';
        }
    }

    goto exit;

err:
    fret = -1;
    if (*ud->smds)
    {
        free(*ud->smds);
        *ud->smds = NULL;
    }
    if (*ud->smdp)
    {
        free(*ud->smdp);
        *ud->smdp = NULL;
    }
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_EuiccConfiguredAddressesResponse, asn1resp);
    }

    return fret;
}

int es10a_get_euicc_configured_addresses(struct euicc_ctx *ctx, char **smdp, char **smds)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    EuiccConfiguredAddressesRequest_t *asn1req = NULL;
    struct userdata_get_euicc_configured_addresses ud;

    asn1req = malloc(sizeof(EuiccConfiguredAddressesRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1erval = der_encode_to_buffer(&asn_DEF_EuiccConfiguredAddressesRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_EuiccConfiguredAddressesRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ud.smdp = smdp;
    ud.smds = smds;

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_EuiccConfiguredAddressesResponse, &ud);
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
        ASN_STRUCT_FREE(asn_DEF_EuiccConfiguredAddressesRequest, asn1req);
    }

    return fret;
}

static int iter_SetDefaultDpAddressResponse(struct apdu_response *response, void *userdata)
{
    long *eresult = (long *)userdata;
    int fret = 0;
    asn_dec_rval_t asn1drval;
    SetDefaultDpAddressResponse_t *asn1resp = NULL;

    asn1drval = ber_decode(NULL, &asn_DEF_SetDefaultDpAddressResponse, (void **)&asn1resp, response->data, response->length);
    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    asn_INTEGER2long(&asn1resp->setDefaultDpAddressResult, eresult);
    goto exit;

err:
    fret = -1;
exit:
    if (asn1resp)
    {
        ASN_STRUCT_FREE(asn_DEF_SetDefaultDpAddressResponse, asn1resp);
    }

    return fret;
}

int es10a_set_default_dp_address(struct euicc_ctx *ctx, const char *smdp)
{
    int fret = 0;
    int ret;
    asn_enc_rval_t asn1erval;
    SetDefaultDpAddressRequest_t *asn1req = NULL;
    unsigned long eresult;

    asn1req = malloc(sizeof(SetDefaultDpAddressRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    ret = OCTET_STRING_fromString(&asn1req->defaultDpAddress, smdp);
    if (ret < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_SetDefaultDpAddressRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_SetDefaultDpAddressRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    ret = es10x_command_iter(ctx, ctx->g_asn1_der_request_buf, asn1erval.encoded, iter_SetDefaultDpAddressResponse, &eresult);
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
        ASN_STRUCT_FREE(asn_DEF_SetDefaultDpAddressRequest, asn1req);
    }

    return fret;
}
