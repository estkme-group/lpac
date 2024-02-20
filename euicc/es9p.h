#pragma once
#include <inttypes.h>
#include "euicc.h"
#include "es10b.h"

struct es9p_ctx
{
    struct euicc_ctx *euicc_ctx;
    const char *address;
    char *transactionId;
    struct
    {
        char subjectCode[8 + 1];
        char reasonCode[8 + 1];
        char subjectIdentifier[128 + 1];
        char message[128 + 1];
    } statusCodeData;
};

struct es11_authenticate_client_resp
{
    char *status;
    void *cjson_array_result;
};

int es9p_InitiateAuthentication(struct es9p_ctx *ctx, struct es10b_authenticate_server_param *resp, const char *b64_euiccChallenge, const char *b64_EUICCInfo1);
int es9p_GetBoundProfilePackage(struct es9p_ctx *ctx, char **b64_BoundProfilePackage, const char *b64_PrepareDownloadResponse);
int es9p_AuthenticateClient(struct es9p_ctx *ctx, struct es10b_prepare_download_param *resp, const char *b64_AuthenticateServerResponse);
int es9p_HandleNotification(struct es9p_ctx *ctx, const char *b64_PendingNotification);
int es9p_CancelSession(struct es9p_ctx *ctx, const char *b64_CancelSessionResponse);
int es11_AuthenticateClient(struct es9p_ctx *ctx, struct es11_authenticate_client_resp *resp, const char *b64_AuthenticateServerResponse);

void es9p_ctx_free(struct es9p_ctx *ctx);
