#pragma once

#include "euicc.h"

enum es10b_profile_management_operation
{
    ES10B_PROFILE_MANAGEMENT_OPERATION_NULL = -1,
    ES10B_PROFILE_MANAGEMENT_OPERATION_INSTALL = 0x80,
    ES10B_PROFILE_MANAGEMENT_OPERATION_ENABLE = 0x40,
    ES10B_PROFILE_MANAGEMENT_OPERATION_DISABLE = 0x20,
    ES10B_PROFILE_MANAGEMENT_OPERATION_DELETE = 0x10,
    ES10B_PROFILE_MANAGEMENT_OPERATION_UNDEFINED = 0xFF,
};

struct es10b_prepare_download_param
{
    const char *b64_profileMetadata;
    const char *b64_smdpSigned2;
    const char *b64_smdpSignature2;
    const char *b64_smdpCertificate;
    const char *confirmationCode;
};

struct es10b_notification_metadata_list
{
    unsigned long seqNumber;
    enum es10b_profile_management_operation profileManagementOperation;
    char *notificationAddress;
    char *iccid;

    struct es10b_notification_metadata_list *next;
};

struct es10b_pending_notification
{
    char *notificationAddress;
    char *b64_PendingNotification;
};

struct es10b_authenticate_server_param
{
    const char *b64_serverSigned1;
    const char *b64_serverSignature1;
    const char *b64_euiccCiPKIdToBeUsed;
    const char *b64_serverCertificate;
    const char *matchingId;
    const char *imei;
};

struct es10b_cancel_session_param
{
    const unsigned char *transactionId;
    unsigned char transactionIdLen;
    enum es10b_cancel_session_reason
    {
        ES10B_CANCEL_SESSION_REASON_ENDUSERREJECTION = 0,
        ES10B_CANCEL_SESSION_REASON_POSTPONED = 1,
        ES10B_CANCEL_SESSION_REASON_TIMEOUT = 2,
        ES10B_CANCEL_SESSION_REASON_PPRNOTALLOWED = 3,
        ES10B_CANCEL_SESSION_REASON_METADATAMISMATCH = 4,
        ES10B_CANCEL_SESSION_REASON_LOADBPPEXECUTIONERROR = 5,
        ES10B_CANCEL_SESSION_REASON_UNDEFINED = 127
    } reason;
};

int es10b_PrepareDownload(struct euicc_ctx *ctx, char **b64_PrepareDownloadResponse, struct es10b_prepare_download_param *param);
int es10b_LoadBoundProfilePackage(struct euicc_ctx *ctx, const char *b64_BoundProfilePackage);
int es10b_GetEUICCChallenge(struct euicc_ctx *ctx, char **b64_euiccChallenge);
int es10b_GetEUICCInfo(struct euicc_ctx *ctx, char **b64_EUICCInfo1);
int es10b_ListNotification(struct euicc_ctx *ctx, struct es10b_notification_metadata_list **notificationMetadataList);
int es10b_RetrieveNotificationsList(struct euicc_ctx *ctx, struct es10b_pending_notification *PendingNotification, unsigned long seqNumber);
int es10b_RemoveNotificationFromList(struct euicc_ctx *ctx, unsigned long seqNumber);
int es10b_AuthenticateServer(struct euicc_ctx *ctx, char **b64_AuthenticateServerResponse, struct es10b_authenticate_server_param *param);
int es10b_CancelSession(struct euicc_ctx *ctx, char **b64_CancelSessionResponse, struct es10b_cancel_session_param *param);

void es10b_notification_metadata_free_all(struct es10b_notification_metadata_list *notificationMetadataList);
void es10b_notification_free(struct es10b_pending_notification *PendingNotification);
