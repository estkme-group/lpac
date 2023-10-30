#pragma once
#include "euicc.h"
#include <inttypes.h>

struct es9p_initiate_authentication_resp
{
    char *status;
    char *transaction_id;
    char *b64_server_signed_1;
    char *b64_server_signature_1;
    char *b64_euicc_ci_pkid_to_be_used;
    char *b64_server_certificate;
};

struct es9p_get_bound_profile_package_resp
{
    char *status;
    char *b64_bpp;
};

struct es9p_authenticate_client_resp
{
    char *status;
    char *b64_smdp_signed_2;
    char *b64_smdp_signature_2;
    char *b64_smdp_certificate;
};

struct es11_authenticate_client_resp
{
    char *status;
    void *cjson_array_result;
};

int es9p_initiate_authentication(struct euicc_ctx *ctx, const char *smdp, const char *b64_euicc_challenge, const char *b64_euicc_info_1, struct es9p_initiate_authentication_resp *resp);
int es9p_get_bound_profile_package(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_prepare_download_response, struct es9p_get_bound_profile_package_resp *resp);
int es9p_authenticate_client(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_authenticate_server_response, struct es9p_authenticate_client_resp *resp);
int es9p_handle_notification(struct euicc_ctx *ctx, const char *smdp, const char *b64_pending_notification);
int es9p_cancel_session(struct euicc_ctx *ctx, const char *smdp, const char *transaction_id, const char *b64_cancel_session_response);

int es11_authenticate_client(struct euicc_ctx *ctx, const char *smds, const char *transaction_id, const char *b64_authenticate_server_response, struct es11_authenticate_client_resp *resp);
