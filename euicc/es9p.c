#include "es9p.h"
#include "es9p_errors.h"

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

static void es9p_base64_trim(char *str)
{
    char *p = str;

    while (*p)
    {
        if (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')
        {
            memmove(p, p + 1, strlen(p));
        }
        else
        {
            p++;
        }
    }
}

static int es9p_trans_ex(struct euicc_ctx *ctx, const char *url, const char *url_postfix, uint32_t *rcode, char **str_rx, const char *str_tx)
{
    int fret = 0;
    uint32_t rcode_mearged;
    uint8_t *rbuf = NULL;
    uint32_t rlen;
    char *full_url = NULL;
    const char *url_prefix = "https://";

    if (!ctx->http.interface)
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
    if (ctx->http.interface->transmit(ctx, full_url, &rcode_mearged, &rbuf, &rlen, (const uint8_t *)str_tx, strlen(str_tx), lpa_header) < 0)
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

static int es9p_trans_json(struct euicc_ctx *ctx, const char *smdp, const char *api, const char *ikey[], const char *idata[], const char *okey[], const char *oobj, void **optr[])
{
    int fret = 0;
    cJSON *sjroot = NULL;
    char *sbuf = NULL;
    uint32_t rcode;
    char *rbuf = NULL;
    cJSON *rjroot = NULL, *rjheader = NULL, *rjfunctionExecutionStatus = NULL;

    strncpy(ctx->http.status.reasonCode, "0.0.0", sizeof(ctx->http.status.reasonCode));
    strncpy(ctx->http.status.subjectCode, "0.0.0", sizeof(ctx->http.status.subjectCode));
    strncpy(ctx->http.status.subjectIdentifier, "unknown", sizeof(ctx->http.status.subjectIdentifier));
    strncpy(ctx->http.status.message, "unknown", sizeof(ctx->http.status.message));

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
        strncpy(ctx->http.status.reasonCode, "0.0.0", sizeof(ctx->http.status.reasonCode));
        strncpy(ctx->http.status.subjectCode, "0.0.0", sizeof(ctx->http.status.subjectCode));
        strncpy(ctx->http.status.subjectIdentifier, "unknown", sizeof(ctx->http.status.subjectIdentifier));
        strncpy(ctx->http.status.message, "HTTP transport failed", sizeof(ctx->http.status.message));
        goto err;
    }
    free(sbuf);
    sbuf = NULL;

    if (rcode / 100 != 2)
    {
        strncpy(ctx->http.status.reasonCode, "0.0.0", sizeof(ctx->http.status.reasonCode));
        strncpy(ctx->http.status.subjectCode, "0.0.0", sizeof(ctx->http.status.subjectCode));
        snprintf(ctx->http.status.subjectIdentifier, sizeof(ctx->http.status.subjectIdentifier), "%d", rcode);
        strncpy(ctx->http.status.message, "HTTP status code error", sizeof(ctx->http.status.message));
        goto err;
    }

    if (!okey)
    {
        fret = 0;
        goto exit;
    }

    if (!(rjroot = cJSON_Parse((const char *)rbuf)))
    {
        strncpy(ctx->http.status.reasonCode, "0.0.0", sizeof(ctx->http.status.reasonCode));
        strncpy(ctx->http.status.subjectCode, "0.0.0", sizeof(ctx->http.status.subjectCode));
        strncpy(ctx->http.status.subjectIdentifier, "root", sizeof(ctx->http.status.subjectIdentifier));
        strncpy(ctx->http.status.message, "Not JSON", sizeof(ctx->http.status.message));
        goto err;
    }
    free(rbuf);
    rbuf = NULL;

    if (!cJSON_IsObject(rjroot))
    {
        strncpy(ctx->http.status.reasonCode, "0.0.0", sizeof(ctx->http.status.reasonCode));
        strncpy(ctx->http.status.subjectCode, "0.0.0", sizeof(ctx->http.status.subjectCode));
        strncpy(ctx->http.status.subjectIdentifier, "root", sizeof(ctx->http.status.subjectIdentifier));
        strncpy(ctx->http.status.message, "Not Object", sizeof(ctx->http.status.message));
        goto err;
    }

    if (!cJSON_HasObjectItem(rjroot, "header"))
    {
        strncpy(ctx->http.status.reasonCode, "0.0.0", sizeof(ctx->http.status.reasonCode));
        strncpy(ctx->http.status.subjectCode, "0.0.0", sizeof(ctx->http.status.subjectCode));
        strncpy(ctx->http.status.subjectIdentifier, "header", sizeof(ctx->http.status.subjectIdentifier));
        strncpy(ctx->http.status.message, "Critical object missing", sizeof(ctx->http.status.message));
        goto err;
    }

    rjheader = cJSON_GetObjectItem(rjroot, "header");

    if (!cJSON_HasObjectItem(rjheader, "functionExecutionStatus"))
    {
        strncpy(ctx->http.status.reasonCode, "0.0.0", sizeof(ctx->http.status.reasonCode));
        strncpy(ctx->http.status.subjectCode, "0.0.0", sizeof(ctx->http.status.subjectCode));
        strncpy(ctx->http.status.subjectIdentifier, "functionExecutionStatus", sizeof(ctx->http.status.subjectIdentifier));
        strncpy(ctx->http.status.message, "Critical object missing", sizeof(ctx->http.status.message));
        goto err;
    }

    rjfunctionExecutionStatus = cJSON_GetObjectItem(rjheader, "functionExecutionStatus");

    if (cJSON_HasObjectItem(rjfunctionExecutionStatus, "statusCodeData"))
    {
        cJSON *statusCodeData = cJSON_GetObjectItem(rjfunctionExecutionStatus, "statusCodeData");

        if (cJSON_HasObjectItem(statusCodeData, "reasonCode") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "reasonCode")))
        {
            strncpy(ctx->http.status.reasonCode, cJSON_GetObjectItem(statusCodeData, "reasonCode")->valuestring, sizeof(ctx->http.status.reasonCode));
        }
        if (cJSON_HasObjectItem(statusCodeData, "subjectCode") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "subjectCode")))
        {
            strncpy(ctx->http.status.subjectCode, cJSON_GetObjectItem(statusCodeData, "subjectCode")->valuestring, sizeof(ctx->http.status.subjectCode));
        }
        if (cJSON_HasObjectItem(statusCodeData, "subjectIdentifier") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "subjectIdentifier")))
        {
            strncpy(ctx->http.status.subjectIdentifier, cJSON_GetObjectItem(statusCodeData, "subjectIdentifier")->valuestring, sizeof(ctx->http.status.subjectIdentifier));
        }
        if (cJSON_HasObjectItem(statusCodeData, "message") && cJSON_IsString(cJSON_GetObjectItem(statusCodeData, "message")))
        {
            strncpy(ctx->http.status.message, cJSON_GetObjectItem(statusCodeData, "message")->valuestring, sizeof(ctx->http.status.message));
        }
        else
        {
            const char* message = es9p_error_message(ctx->http.status.subjectCode, ctx->http.status.reasonCode);
            if (message != NULL)
            {
                strncpy(ctx->http.status.message, message, sizeof(ctx->http.status.message));
            }
            else
            {
                snprintf(ctx->http.status.message, sizeof(ctx->http.status.message), "subject-code: %s, reason-code: %s", ctx->http.status.subjectCode, ctx->http.status.reasonCode);
            }
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

int es9p_initiate_authentication_r(struct euicc_ctx *ctx, char **transaction_id, struct es10b_authenticate_server_param *resp, const char *server_address, const char *b64_euicc_challenge, const char *b64_euicc_info_1)
{
    const char *ikey[] = {"smdpAddress", "euiccChallenge", "euiccInfo1", NULL};
    const char *idata[] = {ctx->http.server_address, b64_euicc_challenge, b64_euicc_info_1, NULL};
    const char *okey[] = {"transactionId", "serverSigned1", "serverSignature1", "euiccCiPKIdToBeUsed", "serverCertificate", NULL};
    const char oobj[] = {0, 0, 0, 0, 0};
    void **optr[] = {(void **)transaction_id, (void **)&resp->b64_serverSigned1, (void **)&resp->b64_serverSignature1, (void **)&resp->b64_euiccCiPKIdToBeUsed, (void **)&resp->b64_serverCertificate, NULL};

    if (es9p_trans_json(ctx, ctx->http.server_address, "/gsma/rsp2/es9plus/initiateAuthentication", ikey, idata, okey, oobj, optr))
    {
        return -1;
    }

    es9p_base64_trim(resp->b64_serverSigned1);
    es9p_base64_trim(resp->b64_serverSignature1);
    es9p_base64_trim(resp->b64_euiccCiPKIdToBeUsed);
    es9p_base64_trim(resp->b64_serverCertificate);

    return 0;
}

int es9p_get_bound_profile_package_r(struct euicc_ctx *ctx, char **b64_bound_profile_package, const char *server_address, const char *transaction_id, const char *b64_prepare_download_response)
{
    const char *ikey[] = {"transactionId", "prepareDownloadResponse", NULL};
    const char *idata[] = {transaction_id, b64_prepare_download_response, NULL};
    const char *okey[] = {"boundProfilePackage", NULL};
    const char oobj[] = {0};
    void **optr[] = {(void **)b64_bound_profile_package, NULL};

    if (es9p_trans_json(ctx, ctx->http.server_address, "/gsma/rsp2/es9plus/getBoundProfilePackage", ikey, idata, okey, oobj, optr))
    {
        return -1;
    }

    es9p_base64_trim(*b64_bound_profile_package);

    return 0;
}

int es9p_authenticate_client_r(struct euicc_ctx *ctx, struct es10b_prepare_download_param *resp, const char *server_address, const char *transaction_id, const char *b64_authenticate_server_response)
{
    const char *ikey[] = {"transactionId", "authenticateServerResponse", NULL};
    const char *idata[] = {transaction_id, b64_authenticate_server_response, NULL};
    const char *okey[] = {"profileMetadata", "smdpSigned2", "smdpSignature2", "smdpCertificate", NULL};
    const char oobj[] = {0, 0, 0, 0};
    void **optr[] = {(void **)&resp->b64_profileMetadata, (void **)&resp->b64_smdpSigned2, (void **)&resp->b64_smdpSignature2, (void **)&resp->b64_smdpCertificate, NULL};

    if (es9p_trans_json(ctx, ctx->http.server_address, "/gsma/rsp2/es9plus/authenticateClient", ikey, idata, okey, oobj, optr))
    {
        return -1;
    }

    es9p_base64_trim(resp->b64_profileMetadata);
    es9p_base64_trim(resp->b64_smdpSigned2);
    es9p_base64_trim(resp->b64_smdpSignature2);
    es9p_base64_trim(resp->b64_smdpCertificate);

    return 0;
}

int es9p_cancel_session_r(struct euicc_ctx *ctx, const char *server_address, const char *transaction_id, const char *b64_cancel_session_response)
{
    const char *ikey[] = {"transactionId", "cancelSessionResponse", NULL};
    const char *idata[] = {transaction_id, b64_cancel_session_response, NULL};

    if (es9p_trans_json(ctx, ctx->http.server_address, "/gsma/rsp2/es9plus/cancelSession", ikey, idata, NULL, NULL, NULL))
    {
        return -1;
    }

    return 0;
}

int es11_authenticate_client_r(struct euicc_ctx *ctx, char ***smdp_list, const char *server_address, const char *transaction_id, const char *b64_authenticate_server_response)
{
    int fret = 0;
    cJSON *j_eventEntries = NULL;
    int j_eventEntries_size = 0;
    const char *ikey[] = {"transactionId", "authenticateServerResponse", NULL};
    const char *idata[] = {transaction_id, b64_authenticate_server_response, NULL};
    const char *okey[] = {"eventEntries", NULL};
    const char oobj[] = {1};
    void **optr[] = {(void **)&j_eventEntries, NULL};

    if (es9p_trans_json(ctx, ctx->http.server_address, "/gsma/rsp2/es9plus/authenticateClient", ikey, idata, okey, oobj, optr))
    {
        return -1;
    }

    if (j_eventEntries == NULL || !cJSON_IsArray(j_eventEntries))
    {
        return -1;
    }

    j_eventEntries_size = cJSON_GetArraySize(j_eventEntries);

    *smdp_list = malloc(sizeof(char *) * (j_eventEntries_size + 1));
    if (*smdp_list == NULL)
    {
        fret = -1;
        goto err;
    }
    memset(*smdp_list, 0, sizeof(char *) * (j_eventEntries_size + 1));

    for (int i = 0; i < j_eventEntries_size; i++)
    {
        cJSON *j_event = cJSON_GetArrayItem(j_eventEntries, i);
        cJSON *j_eventType = cJSON_GetObjectItem(j_event, "rspServerAddress");

        if (j_eventType == NULL || !cJSON_IsString(j_eventType))
        {
            fret = -1;
            goto err;
        }

        (*smdp_list)[i] = strdup(j_eventType->valuestring);
    }

    fret = 0;
    goto exit;

err:
    if (*smdp_list)
    {
        for (int i = 0; i < j_eventEntries_size; i++)
        {
            free((*smdp_list)[i]);
        }
        free(*smdp_list);
        *smdp_list = NULL;
    }

exit:
    cJSON_Delete(j_eventEntries);
    return fret;
}

int es9p_initiate_authentication(struct euicc_ctx *ctx)
{
    int fret;

    if (ctx->http._internal.authenticate_server_param)
    {
        return -1;
    }

    if (ctx->http._internal.b64_euicc_challenge == NULL)
    {
        return -1;
    }

    if (ctx->http._internal.b64_euicc_info_1 == NULL)
    {
        return -1;
    }

    ctx->http._internal.authenticate_server_param = malloc(sizeof(struct es10b_authenticate_server_param));
    if (ctx->http._internal.authenticate_server_param == NULL)
    {
        return -1;
    }

    fret = es9p_initiate_authentication_r(ctx, &ctx->http._internal.transaction_id_http, ctx->http._internal.authenticate_server_param, ctx->http.server_address, ctx->http._internal.b64_euicc_challenge, ctx->http._internal.b64_euicc_info_1);
    if (fret < 0)
    {
        free(ctx->http._internal.authenticate_server_param);
        ctx->http._internal.authenticate_server_param = NULL;
        return fret;
    }

    free(ctx->http._internal.b64_euicc_challenge);
    ctx->http._internal.b64_euicc_challenge = NULL;

    free(ctx->http._internal.b64_euicc_info_1);
    ctx->http._internal.b64_euicc_info_1 = NULL;

    return fret;
}

int es9p_get_bound_profile_package(struct euicc_ctx *ctx)
{
    int fret;

    if (ctx->http._internal.b64_bound_profile_package)
    {
        return -1;
    }

    if (ctx->http._internal.b64_prepare_download_response == NULL)
    {
        return -1;
    }

    fret = es9p_get_bound_profile_package_r(ctx, &ctx->http._internal.b64_bound_profile_package, ctx->http.server_address, ctx->http._internal.transaction_id_http, ctx->http._internal.b64_prepare_download_response);
    if (fret < 0)
    {
        free(ctx->http._internal.b64_bound_profile_package);
        ctx->http._internal.b64_bound_profile_package = NULL;
        return fret;
    }

    free(ctx->http._internal.b64_prepare_download_response);
    ctx->http._internal.b64_prepare_download_response = NULL;

    return fret;
}

int es9p_authenticate_client(struct euicc_ctx *ctx)
{
    int fret;

    if (ctx->http._internal.prepare_download_param)
    {
        return -1;
    }

    if (ctx->http._internal.b64_authenticate_server_response == NULL)
    {
        return -1;
    }

    ctx->http._internal.prepare_download_param = malloc(sizeof(struct es10b_prepare_download_param));
    if (ctx->http._internal.prepare_download_param == NULL)
    {
        return -1;
    }

    fret = es9p_authenticate_client_r(ctx, ctx->http._internal.prepare_download_param, ctx->http.server_address, ctx->http._internal.transaction_id_http, ctx->http._internal.b64_authenticate_server_response);
    if (fret < 0)
    {
        free(ctx->http._internal.prepare_download_param);
        ctx->http._internal.prepare_download_param = NULL;
        return fret;
    }

    free(ctx->http._internal.b64_authenticate_server_response);
    ctx->http._internal.b64_authenticate_server_response = NULL;

    return fret;
}

int es9p_cancel_session(struct euicc_ctx *ctx)
{
    int fret;

    if (ctx->http._internal.b64_cancel_session_response == NULL)
    {
        return -1;
    }

    fret = es9p_cancel_session_r(ctx, ctx->http.server_address, ctx->http._internal.transaction_id_http, ctx->http._internal.b64_cancel_session_response);
    if (fret < 0)
    {
        return fret;
    }

    free(ctx->http._internal.b64_cancel_session_response);
    ctx->http._internal.b64_cancel_session_response = NULL;

    return fret;
}

int es11_authenticate_client(struct euicc_ctx *ctx, char ***smdp_list)
{
    int fret;

    if (ctx->http._internal.b64_authenticate_server_response == NULL)
    {
        return -1;
    }

    fret = es11_authenticate_client_r(ctx, smdp_list, ctx->http.server_address, ctx->http._internal.transaction_id_http, ctx->http._internal.b64_authenticate_server_response);
    if (fret < 0)
    {
        return fret;
    }

    free(ctx->http._internal.b64_authenticate_server_response);
    ctx->http._internal.b64_authenticate_server_response = NULL;

    return fret;
}

int es9p_handle_notification(struct euicc_ctx *ctx, const char *b64_PendingNotification)
{
    const char *ikey[] = {"pendingNotification", NULL};
    const char *idata[] = {b64_PendingNotification, NULL};

    return es9p_trans_json(ctx, ctx->http.server_address, "/gsma/rsp2/es9plus/handleNotification", ikey, idata, NULL, NULL, NULL);
}

void es11_smdp_list_free_all(char **smdp_list)
{
    if (smdp_list)
    {
        for (int i = 0; smdp_list[i] != NULL; i++)
        {
            free(smdp_list[i]);
        }
        free(smdp_list);
    }
}
