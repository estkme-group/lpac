#include "es9p.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

static int es9p_trans_ex(struct euicc_ctx *ctx, const char *url, const char *url_postfix, unsigned int *rcode, char **str_rx, const char *str_tx)
{
    int fret = 0;
    uint32_t rcode_mearged;
    uint8_t *rbuf = NULL;
    uint32_t rlen;
    char *full_url = NULL;
    const char *url_prefix = "https://";

    if (!ctx->interface.es9p.transmit)
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
    if (ctx->interface.es9p.transmit(full_url, &rcode_mearged, &rbuf, &rlen, str_tx, strlen(str_tx)) < 0)
    {
        goto err;
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

static int es9p_trans_json(struct euicc_ctx *ctx, const char *smdp, const char *api, const char *ikey[], const char *idata[], const char *okey[], char **optr[], char **status)
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
        if (!cJSON_AddStringToObject(sjroot, ikey[i], idata[i]))
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
    if (okey)
    {
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
            cJSON *header;
            cJSON *functionExecutionStatus;
            cJSON *statusCodeData;
            if (header = cJSON_GetObjectItem(rjroot, "header"))
            {
                if (cJSON_HasObjectItem(header, "functionExecutionStatus"))
                {
                    if (functionExecutionStatus = cJSON_GetObjectItem(header, "functionExecutionStatus"))
                    {
                        if (cJSON_HasObjectItem(functionExecutionStatus, "statusCodeData"))
                        {
                            if (statusCodeData = cJSON_GetObjectItem(functionExecutionStatus, "statusCodeData"))
                            {
                                if (status)
                                {
                                    free(*status);
                                    *status = cJSON_PrintUnformatted(statusCodeData);
                                }
                            }
                        }
                    }
                }
            }
        }

        for (int i = 0; okey[i] != NULL; i++)
        {
            if (!cJSON_HasObjectItem(rjroot, okey[i]))
            {
                goto err;
            }
            if (!cJSON_IsString(cJSON_GetObjectItem(rjroot, okey[i])))
            {
                goto err;
            }
            if (!(*optr[i] = strdup(cJSON_GetObjectItem(rjroot, okey[i])->valuestring)))
            {
                goto err;
            }
        }

        cJSON_Delete(rjroot);
        rjroot = NULL;
    }

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
    char **optr[] = {&resp->transaction_id, &resp->b64_server_signed_1, &resp->b64_server_signature_1, &resp->b64_euicc_ci_pkid_to_be_used, &resp->b64_server_certificate, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/initiateAuthentication", ikey, idata, okey, optr, &resp->status);
}

int es9p_get_bound_profile_package(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_prepare_download_response, struct es9p_get_bound_profile_package_resp *resp)
{
    const char *ikey[] = {"transactionId", "prepareDownloadResponse", NULL};
    const char *idata[] = {transaction_id, b64_prepare_download_response, NULL};
    const char *okey[] = {"boundProfilePackage", NULL};
    char **optr[] = {&resp->b64_bpp, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/getBoundProfilePackage", ikey, idata, okey, optr, &resp->status);
}

int es9p_authenticate_client(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_authenticate_server_response, struct es9p_authenticate_client_resp *resp)
{
    const char *ikey[] = {"transactionId", "authenticateServerResponse", NULL};
    const char *idata[] = {transaction_id, b64_authenticate_server_response, NULL};
    const char *okey[] = {"smdpSigned2", "smdpSignature2", "smdpCertificate", NULL};
    char **optr[] = {&resp->b64_smdp_signed_2, &resp->b64_smdp_signature_2, &resp->b64_smdp_certificate, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/authenticateClient", ikey, idata, okey, optr, &resp->status);
}

int es9p_handle_notification(struct euicc_ctx *ctx, const char *smdp, const char *b64_pending_notification)
{
    const char *ikey[] = {"pendingNotification", NULL};
    const char *idata[] = {b64_pending_notification, NULL};

    return es9p_trans_json(ctx, smdp, "/gsma/rsp2/es9plus/handleNotification", ikey, idata, NULL, NULL, NULL);
}

int es9p_cancel_session(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_cancel_session_response)
{
    // /gsma/rsp2/es9plus/cancelSession
}
