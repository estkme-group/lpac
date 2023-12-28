#include "es9p.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON_ex.h>

static const char *lpa_header[] = {
    "User-Agent: gsma-rsp-lpad",
    "X-Admin-Protocol: gsma/rsp/v2.2.0",
    "Content-Type: application/json",
    NULL,
};

static int es9p_trans_ex(struct euicc_ctx *ctx, const char *url, const char *url_postfix, unsigned int *rcode, char **str_rx, const char *str_tx)
{
    int fret = 0;
    uint32_t rcode_mearged;
    uint8_t *rbuf = NULL;
    uint32_t rlen;
    char *full_url = NULL;
    const char *url_prefix = "https://";

    if (!ctx->interface.http)
    {
        goto err;
    }

    full_url = malloc(strlen(url_prefix) + strlen(url) + strlen(url_postfix) + 1);
    if (full_url == NULL)
    {
        goto err;
    }

    full_url[0] = '\0';
    strcat(full_url, url_prefix);
    strcat(full_url, url);
    strcat(full_url, url_postfix);

    if (getenv("LIBEUICC_DEBUG_HTTP"))
    {
        fprintf(stderr, "[DEBUG] [HTTP] [TX] url: %s, data: %s\n", full_url, str_tx);
    }
    if (ctx->interface.http->transmit(ctx, full_url, &rcode_mearged, &rbuf, &rlen, (const uint8_t *)str_tx, strlen(str_tx), lpa_header) < 0)
    {
        goto err;
    }
    if (getenv("LIBEUICC_DEBUG_HTTP"))
    {
        fprintf(stderr, "[DEBUG] [HTTP] [RX] rcode: %d, data: %s\n", rcode_mearged, rbuf);
    }

    free(full_url);
    full_url = NULL;

    *str_rx = malloc(rlen + 1);
    if (*str_rx == NULL)
    {
        goto err;
    }
    memcpy(*str_rx, rbuf, rlen);
    (*str_rx)[rlen] = '\0';

    free(rbuf);
    rbuf = NULL;

    *rcode = rcode_mearged;

    fret = 0;
    goto exit;

err:
    fret = -1;
exit:
    free(full_url);
    free(rbuf);
    return fret;
}

static int es9p_trans_json(struct euicc_ctx *ctx, const char *smdp, const char *api, const char *ikey[], const char *idata[], const char *okey[], const char *oobj, void **optr[], char **status)
{
    int fret = 0;
    cJSON *sjroot = NULL;
    char *sbuf = NULL;
    unsigned int rcode;
    char *rbuf = NULL;
    cJSON *rjroot = NULL;

    if (status)
    {
        *status = strdup("Internal error");
    }

    if (!(sjroot = cJSON_CreateObject()))
    {
        goto err;
    }

    for (int i = 0; ikey[i] != NULL; i++)
    {
        if (!cJSON_AddStringOrNullToObject(sjroot, ikey[i], idata[i]))
        {
            goto err;
        }
    }

    if (!(sbuf = cJSON_PrintUnformatted(sjroot)))
    {
        goto err;
    }
    cJSON_Delete(sjroot);
    sjroot = NULL;

    if (es9p_trans_ex(ctx, smdp, api, &rcode, &rbuf, sbuf) < 0)
    {
        if (status)
        {
            free(*status);
            *status = strdup("Transport failed");
        }
        goto err;
    }
    free(sbuf);
    sbuf = NULL;

    if (rcode / 100 != 2)
    {
        if (status)
        {
            free(*status);
            *status = strdup("Status code error");
        }
        goto err;
    }

    if (!okey)
    {
        fret = 0;
        goto exit;
    }

    if (!(rjroot = cJSON_Parse((const char *)rbuf)))
    {
        goto err;
    }
    free(rbuf);
    rbuf = NULL;

    if (!cJSON_IsObject(rjroot))
    {
        goto err;
    }

    if (status)
    {
        free(*status);
        *status = strdup("functionExecutionStatus unknown");
    }

    if (cJSON_HasObjectItem(rjroot, "header"))
    {
        while (1)
        {
            cJSON *node;
            if (!(node = cJSON_GetObjectItem(rjroot, "header")))
            {
                break;
            }
            if (!(node = cJSON_GetObjectItem(node, "functionExecutionStatus")))
            {
                break;
            }
            if (!(node = cJSON_GetObjectItem(node, "statusCodeData")))
            {
                break;
            }
            if (status)
            {
                free(*status);
                *status = cJSON_PrintUnformatted(node);
            }
            break;
        }
    }

    for (int i = 0; okey[i] != NULL; i++)
    {
        cJSON *obj;

        obj = cJSON_GetObjectItem(rjroot, okey[i]);
        if (!obj)
        {
            goto err;
        }

        if (cJSON_IsString(obj))
        {
            if (!(*optr[i] = strdup(obj->valuestring)))
            {
                goto err;
            }
        }
        else
        {
            if (oobj[i] == 0)
            {
                goto err;
            }
            if (!(*(optr[i]) = cJSON_Duplicate(obj, 1)))
            {
                goto err;
            }
        }
    }

    cJSON_Delete(rjroot);
    rjroot = NULL;

    fret = 0;
    goto exit;

err:
    fret = -1;
exit:
    free(sbuf);
    cJSON_Delete(sjroot);
    free(rbuf);
    cJSON_Delete(rjroot);
    return fret;
}

int es9p_initiate_authentication(struct euicc_ctx *ctx, const char *smdp, const char *b64_euicc_challenge, const char *b64_euicc_info_1, struct es9p_initiate_authentication_resp *resp)
{
    const char *ikey[] = {"smdpAddress", "euiccChallenge", "euiccInfo1", NULL};
    const char *idata[] = {smdp, b64_euicc_challenge, b64_euicc_info_1, NULL};
    const char *okey[] = {"transactionId", "serverSigned1", "serverSignature1", "euiccCiPKIdToBeUsed", "serverCertificate", NULL};
    const char oobj[] = {0, 0, 0, 0, 0};
    void **optr[] = {(void **)&resp->transaction_id, (void **)&resp->b64_server_signed_1, (void **)&resp->b64_server_signature_1, (void **)&resp->b64_euicc_ci_pkid_to_be_used, (void **)&resp->b64_server_certificate, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/initiateAuthentication", ikey, idata, okey, oobj, optr, &resp->status);
}

int es9p_get_bound_profile_package(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_prepare_download_response, struct es9p_get_bound_profile_package_resp *resp)
{
    const char *ikey[] = {"transactionId", "prepareDownloadResponse", NULL};
    const char *idata[] = {transaction_id, b64_prepare_download_response, NULL};
    const char *okey[] = {"boundProfilePackage", NULL};
    const char oobj[] = {0};
    void **optr[] = {(void **)&resp->b64_bpp, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/getBoundProfilePackage", ikey, idata, okey, oobj, optr, &resp->status);
}

int es9p_authenticate_client(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_authenticate_server_response, struct es9p_authenticate_client_resp *resp)
{
    const char *ikey[] = {"transactionId", "authenticateServerResponse", NULL};
    const char *idata[] = {transaction_id, b64_authenticate_server_response, NULL};
    const char *okey[] = {"smdpSigned2", "smdpSignature2", "smdpCertificate", NULL};
    const char oobj[] = {0, 0, 0};
    void **optr[] = {(void **)&resp->b64_smdp_signed_2, (void **)&resp->b64_smdp_signature_2, (void **)&resp->b64_smdp_certificate, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/authenticateClient", ikey, idata, okey, oobj, optr, &resp->status);
}

int es9p_handle_notification(struct euicc_ctx *ctx, const char *smdp, const char *b64_pending_notification)
{
    const char *ikey[] = {"pendingNotification", NULL};
    const char *idata[] = {b64_pending_notification, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/handleNotification", ikey, idata, NULL, NULL, NULL, NULL);
}

int es9p_cancel_session(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_cancel_session_response)
{
    // /gsma/rsp2/es9plus/cancelSession
    return -1;
}

int es11_authenticate_client(struct euicc_ctx *ctx, const char *smds, const char *transaction_id, const char *b64_authenticate_server_response, struct es11_authenticate_client_resp *resp)
{
    const char *ikey[] = {"transactionId", "authenticateServerResponse", NULL};
    const char *idata[] = {transaction_id, b64_authenticate_server_response, NULL};
    const char *okey[] = {"eventEntries", NULL};
    const char oobj[] = {1};
    void **optr[] = {(void **)&resp->cjson_array_result, NULL};

    resp->cjson_array_result = NULL;
    return es9p_trans_json(ctx, smds, "/gsma/rsp2/es9plus/authenticateClient", ikey, idata, okey, oobj, optr, &resp->status);
}
