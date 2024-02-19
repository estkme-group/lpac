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

struct es9p_get_bound_profile_package_resp
{
    char *b64_bpp;
};

struct es11_authenticate_client_resp
{
    char *status;
    void *cjson_array_result;
};

int es9p_initiate_authentication(struct es9p_ctx *ctx, struct es10b_AuthenticateServer_param *resp, const char *b64_euicc_challenge, const char *b64_euicc_info_1);
int es9p_get_bound_profile_package(struct es9p_ctx *ctx, struct es9p_get_bound_profile_package_resp *resp, const char *b64_prepare_download_response);
int es9p_authenticate_client(struct es9p_ctx *ctx, struct es10b_PrepareDownload_param *resp, const char *b64_authenticate_server_response);
int es9p_handle_notification(struct es9p_ctx *ctx, const char *b64_pending_notification);

int es9p_cancel_session(struct es9p_ctx *ctx, const char *b64_cancel_session_response);

int es11_authenticate_client(struct es9p_ctx *ctx, struct es11_authenticate_client_resp *resp, const char *b64_authenticate_server_response);
