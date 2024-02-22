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

static int es9p_trans_ex(struct es9p_ctx *ctx, const char *url, const char *url_postfix, uint32_t *rcode, char **str_rx, const char *str_tx)
{
    int fret = 0;
    uint32_t rcode_mearged;
    uint8_t *rbuf = NULL;
    uint32_t rlen;
    char *full_url = NULL;
    const char *url_prefix = "https://";

    if (!ctx->euicc_ctx->interface.http)
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
    if (ctx->euicc_ctx->interface.http->transmit(ctx->euicc_ctx, full_url, &rcode_mearged, &rbuf, &rlen, (const uint8_t *)str_tx, strlen(str_tx), lpa_header) < 0)
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

static int es9p_trans_json(struct es9p_ctx *ctx, const char *smdp, const char *api, const char *ikey[], const char *idata[], const char *okey[], const char *oobj, void **optr[])
{
    int fret = 0;
    cJSON *sjroot = NULL;
    char *sbuf = NULL;
    uint32_t rcode;
    char *rbuf = NULL;
    cJSON *rjroot = NULL, *rjheader = NULL, *rjfunctionExecutionStatus = NULL;

    strncpy(ctx->statusCodeData.reasonCode, "0.0.0", sizeof(ctx->statusCodeData.reasonCode));
    strncpy(ctx->statusCodeData.subjectCode, "0.0.0", sizeof(ctx->statusCodeData.subjectCode));
    strncpy(ctx->statusCodeData.subjectIdentifier, "unknown", sizeof(ctx->statusCodeData.subjectIdentifier));
    strncpy(ctx->statusCodeData.message, "unknown", sizeof(ctx->statusCodeData.message));

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
        strncpy(ctx->statusCodeData.reasonCode, "0.0.0", sizeof(ctx->statusCodeData.reasonCode));
        strncpy(ctx->statusCodeData.subjectCode, "0.0.0", sizeof(ctx->statusCodeData.subjectCode));
        strncpy(ctx->statusCodeData.subjectIdentifier, "unknown", sizeof(ctx->statusCodeData.subjectIdentifier));
        strncpy(ctx->statusCodeData.message, "HTTP transport failed", sizeof(ctx->statusCodeData.message));
        goto err;
    }
    free(sbuf);
    sbuf = NULL;

    if (rcode / 100 != 2)
    {
        strncpy(ctx->statusCodeData.reasonCode, "0.0.0", sizeof(ctx->statusCodeData.reasonCode));
        strncpy(ctx->statusCodeData.subjectCode, "0.0.0", sizeof(ctx->statusCodeData.subjectCode));
        snprintf(ctx->statusCodeData.subjectIdentifier, sizeof(ctx->statusCodeData.subjectIdentifier), "%d", rcode);
        strncpy(ctx->statusCodeData.message, "HTTP status code error", sizeof(ctx->statusCodeData.message));
        goto err;
    }

    if (!okey)
    {
        fret = 0;
        goto exit;
    }

    if (!(rjroot = cJSON_Parse((const char *)rbuf)))
    {
        strncpy(ctx->statusCodeData.reasonCode, "0.0.0", sizeof(ctx->statusCodeData.reasonCode));
        strncpy(ctx->statusCodeData.subjectCode, "0.0.0", sizeof(ctx->statusCodeData.subjectCode));
        strncpy(ctx->statusCodeData.subjectIdentifier, "root", sizeof(ctx->statusCodeData.subjectIdentifier));
        strncpy(ctx->statusCodeData.message, "Not JSON", sizeof(ctx->statusCodeData.message));
        goto err;
    }
    free(rbuf);
    rbuf = NULL;

    if (!cJSON_IsObject(rjroot))
    {
        strncpy(ctx->statusCodeData.reasonCode, "0.0.0", sizeof(ctx->statusCodeData.reasonCode));
        strncpy(ctx->statusCodeData.subjectCode, "0.0.0", sizeof(ctx->statusCodeData.subjectCode));
        strncpy(ctx->statusCodeData.subjectIdentifier, "root", sizeof(ctx->statusCodeData.subjectIdentifier));
        strncpy(ctx->statusCodeData.message, "Not Object", sizeof(ctx->statusCodeData.message));
        goto err;
    }

    if (!cJSON_HasObjectItem(rjroot, "header"))
    {
        strncpy(ctx->statusCodeData.reasonCode, "0.0.0", sizeof(ctx->statusCodeData.reasonCode));
        strncpy(ctx->statusCodeData.subjectCode, "0.0.0", sizeof(ctx->statusCodeData.subjectCode));
        strncpy(ctx->statusCodeData.subjectIdentifier, "header", sizeof(ctx->statusCodeData.subjectIdentifier));
        strncpy(ctx->statusCodeData.message, "Critical object missing", sizeof(ctx->statusCodeData.message));
        goto err;
    }

    rjheader = cJSON_GetObjectItem(rjroot, "header");

    if (!cJSON_HasObjectItem(rjheader, "functionExecutionStatus"))
    {
        strncpy(ctx->statusCodeData.reasonCode, "0.0.0", sizeof(ctx->statusCodeData.reasonCode));
        strncpy(ctx->statusCodeData.subjectCode, "0.0.0", sizeof(ctx->statusCodeData.subjectCode));
        strncpy(ctx->statusCodeData.subjectIdentifier, "functionExecutionStatus", sizeof(ctx->statusCodeData.subjectIdentifier));
        strncpy(ctx->statusCodeData.message, "Critical object missing", sizeof(ctx->statusCodeData.message));
        goto err;
    }

    rjfunctionExecutionStatus = cJSON_GetObjectItem(rjheader, "functionExecutionStatus");

    if (cJSON_HasObjectItem(rjfunctionExecutionStatus, "statusCodeData"))
    {
        cJSON *statusCodeData = cJSON_GetObjectItem(rjfunctionExecutionStatus, "statusCodeData");

        if (cJSON_HasObjectItem(statusCodeData, "reasonCode") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "reasonCode")))
        {
            strncpy(ctx->statusCodeData.reasonCode, cJSON_GetObjectItem(statusCodeData, "reasonCode")->valuestring, sizeof(ctx->statusCodeData.reasonCode));
        }
        if (cJSON_HasObjectItem(statusCodeData, "subjectCode") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "subjectCode")))
        {
            strncpy(ctx->statusCodeData.subjectCode, cJSON_GetObjectItem(statusCodeData, "subjectCode")->valuestring, sizeof(ctx->statusCodeData.subjectCode));
        }
        if (cJSON_HasObjectItem(statusCodeData, "subjectIdentifier") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "subjectIdentifier")))
        {
            strncpy(ctx->statusCodeData.subjectIdentifier, cJSON_GetObjectItem(statusCodeData, "subjectIdentifier")->valuestring, sizeof(ctx->statusCodeData.subjectIdentifier));
        }
        if (cJSON_HasObjectItem(statusCodeData, "message") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "message")))
        {
            strncpy(ctx->statusCodeData.message, cJSON_GetObjectItem(statusCodeData, "message")->valuestring, sizeof(ctx->statusCodeData.message));
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

int es9p_initiate_authentication(struct es9p_ctx *ctx, struct es10b_authenticate_server_param *resp, const char *b64_euiccChallenge, const char *b64_EUICCInfo1)
{
    const char *ikey[] = {"smdpAddress", "euiccChallenge", "euiccInfo1", NULL};
    const char *idata[] = {ctx->address, b64_euiccChallenge, b64_EUICCInfo1, NULL};
    const char *okey[] = {"transactionId", "serverSigned1", "serverSignature1", "euiccCiPKIdToBeUsed", "serverCertificate", NULL};
    const char oobj[] = {0, 0, 0, 0, 0};
    void **optr[] = {(void **)&ctx->transactionId, (void **)&resp->b64_serverSigned1, (void **)&resp->b64_serverSignature1, (void **)&resp->b64_euiccCiPKIdToBeUsed, (void **)&resp->b64_serverCertificate, NULL};

    return es9p_trans_json(ctx, ctx->address, "/gsma/rsp2/es9plus/initiateAuthentication", ikey, idata, okey, oobj, optr);
}

int es9p_get_bound_profile_package(struct es9p_ctx *ctx, char **b64_BoundProfilePackage, const char *b64_PrepareDownloadResponse)
{
    const char *ikey[] = {"transactionId", "prepareDownloadResponse", NULL};
    const char *idata[] = {ctx->transactionId, b64_PrepareDownloadResponse, NULL};
    const char *okey[] = {"boundProfilePackage", NULL};
    const char oobj[] = {0};
    void **optr[] = {(void **)b64_BoundProfilePackage, NULL};

    return es9p_trans_json(ctx, ctx->address, "/gsma/rsp2/es9plus/getBoundProfilePackage", ikey, idata, okey, oobj, optr);
}

int es9p_authenticate_client(struct es9p_ctx *ctx, struct es10b_prepare_download_param *resp, const char *b64_AuthenticateServerResponse)
{
    const char *ikey[] = {"transactionId", "authenticateServerResponse", NULL};
    const char *idata[] = {ctx->transactionId, b64_AuthenticateServerResponse, NULL};
    const char *okey[] = {"profileMetadata", "smdpSigned2", "smdpSignature2", "smdpCertificate", NULL};
    const char oobj[] = {0, 0, 0, 0};
    void **optr[] = {(void **)&resp->b64_profileMetadata, (void **)&resp->b64_smdpSigned2, (void **)&resp->b64_smdpSignature2, (void **)&resp->b64_smdpCertificate, NULL};

    return es9p_trans_json(ctx, ctx->address, "/gsma/rsp2/es9plus/authenticateClient", ikey, idata, okey, oobj, optr);
}

int es9p_handle_notification(struct es9p_ctx *ctx, const char *b64_PendingNotification)
{
    const char *ikey[] = {"pendingNotification", NULL};
    const char *idata[] = {b64_PendingNotification, NULL};

    return es9p_trans_json(ctx, ctx->address, "/gsma/rsp2/es9plus/handleNotification", ikey, idata, NULL, NULL, NULL);
}

int es9p_cancel_session(struct es9p_ctx *ctx, const char *b64_CancelSessionResponse)
{
    const char *ikey[] = {"transactionId", "cancelSessionResponse", NULL};
    const char *idata[] = {ctx->transactionId, b64_CancelSessionResponse, NULL};

    return es9p_trans_json(ctx, ctx->address, "/gsma/rsp2/es9plus/cancelSession", ikey, idata, NULL, NULL, NULL);
}

int es11_authenticate_client(struct es9p_ctx *ctx, struct es11_authenticate_client_resp *resp, const char *b64_AuthenticateServerResponse)
{
    const char *ikey[] = {"transactionId", "authenticateServerResponse", NULL};
    const char *idata[] = {ctx->transactionId, b64_AuthenticateServerResponse, NULL};
    const char *okey[] = {"eventEntries", NULL};
    const char oobj[] = {1};
    void **optr[] = {(void **)&resp->cjson_array_result, NULL};

    resp->cjson_array_result = NULL;
    return es9p_trans_json(ctx, ctx->address, "/gsma/rsp2/es9plus/authenticateClient", ikey, idata, okey, oobj, optr);
}

void es9p_ctx_free(struct es9p_ctx *ctx)
{
    free(ctx->transactionId);
    ctx->transactionId = NULL;
}
