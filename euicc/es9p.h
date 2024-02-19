#pragma once
#include <inttypes.h>
#include "euicc.h"
#include "es10b.h"

struct es9p_ctx
{
    const struct euicc_ctx *euicc_ctx;
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

struct es11_AuthenticateClient_resp
{
    char *status;
    void *cjson_array_result;
};

int es9p_InitiateAuthentication(struct es9p_ctx *ctx, struct es10b_AuthenticateServer_param *resp, const char *b64_euiccChallenge, const char *b64_EUICCInfo1);
int es9p_GetBoundProfilePackage(struct es9p_ctx *ctx, char **b64_BoundProfilePackage, const char *b64_PrepareDownloadResponse);
int es9p_AuthenticateClient(struct es9p_ctx *ctx, struct es10b_PrepareDownload_param *resp, const char *b64_AuthenticateServerResponse);
int es9p_HandleNotification(struct es9p_ctx *ctx, const char *b64_PendingNotification);
int es9p_CancelSession(struct es9p_ctx *ctx, const char *b64_CancelSessionResponse);

int es11_AuthenticateClient(struct es9p_ctx *ctx, struct es11_AuthenticateClient_resp *resp, const char *b64_AuthenticateServerResponse);
