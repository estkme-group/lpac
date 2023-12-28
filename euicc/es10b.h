#pragma once

#include "euicc.h"

struct es10b_prepare_download_param
{
    const char *b64_smdp_signed_2;
    const char *b64_smdp_signature_2;
    const char *b64_smdp_certificate;
    const char *str_checkcode;
    const char *hexstr_transcation_id;
};

struct es10b_notification_metadata
{
    unsigned long seqNumber;
    char *profileManagementOperation;
    char *notificationAddress;
    char *iccid;
};

struct es10b_authenticate_server_param
{
    const char *b64_server_signed_1;
    const char *b64_server_signature_1;
    const char *b64_euicc_ci_pkid_to_be_used;
    const char *b64_server_certificate;
    const char *matchingId;
    const char *imei;
    const unsigned char *tac;
};

enum es10b_cancel_session_reason
{
    ES10B_CANCEL_SESSION_REASON_ENDUSERREJECTION = 0,
    ES10B_CANCEL_SESSION_REASON_POSTPONED = 1,
    ES10B_CANCEL_SESSION_REASON_TIMEOUT = 2,
    ES10B_CANCEL_SESSION_REASON_PPRNOTALLOWED = 3,
    ES10B_CANCEL_SESSION_REASON_METADATAMISMATCH = 4,
    ES10B_CANCEL_SESSION_REASON_LOADBPPEXECUTIONERROR = 5,
    ES10B_CANCEL_SESSION_REASON_UNDEFINED = 127
};

int es10b_prepare_download(struct euicc_ctx *ctx, char **b64_response, struct es10b_prepare_download_param *param);
int es10b_load_bound_profile_package(struct euicc_ctx *ctx, const char *b64_bpp);
int es10b_get_euicc_challenge(struct euicc_ctx *ctx, char **b64_payload);
int es10b_get_euicc_info(struct euicc_ctx *ctx, char **b64_payload);
int es10b_list_notification(struct euicc_ctx *ctx, struct es10b_notification_metadata **metadatas, int *count);
int es10b_retrieve_notification(struct euicc_ctx *ctx, char **b64_payload, char **receiver, unsigned long seqNumber);
int es10b_remove_notification_from_list(struct euicc_ctx *ctx, unsigned long seqNumber);
int es10b_authenticate_server(struct euicc_ctx *ctx, char **b64_response, struct es10b_authenticate_server_param *param);
int es10b_cancel_session(struct euicc_ctx *ctx, const unsigned char *transactionId, unsigned char transactionIdLen, unsigned char reason);

void es10b_notification_metadata_free_all(struct es10b_notification_metadata *notifications, int count);
void es10b_notification_metadata_print(struct es10b_notification_metadata *n);
