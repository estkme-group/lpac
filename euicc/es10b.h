#pragma once

#include "euicc.h"

struct es10b_PrepareDownload_param
{
    const char *b64_profileMetadata;
    const char *b64_smdpSigned2;
    const char *b64_smdpSignature2;
    const char *b64_smdpCertificate;
    const char *str_confirmationCode;
};

struct es10b_notification_metadata
{
    unsigned long seqNumber;
    const char *profileManagementOperation;
    char *notificationAddress;
    char *iccid;

    struct es10b_notification_metadata *next;
};

struct es10b_notification
{
    char *notificationAddress;
    char *b64_payload;
};

struct es10b_AuthenticateServer_param
{
    const char *b64_serverSigned1;
    const char *b64_serverSignature1;
    const char *b64_euiccCiPKIdToBeUsed;
    const char *b64_serverCertificate;
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

int es10b_PrepareDownload(struct euicc_ctx *ctx, char **b64_response, struct es10b_PrepareDownload_param *param);
int es10b_LoadBoundProfilePackage(struct euicc_ctx *ctx, const char *b64_bpp);
int es10b_GetEUICCChallenge(struct euicc_ctx *ctx, char **b64_payload);
int es10b_GetEUICCInfo(struct euicc_ctx *ctx, char **b64_payload);
int es10b_ListNotification(struct euicc_ctx *ctx, struct es10b_notification_metadata **metadatas);
int es10b_RetrieveNotificationsList(struct euicc_ctx *ctx, struct es10b_notification *notification, unsigned long seqNumber);
int es10b_RemoveNotificationFromList(struct euicc_ctx *ctx, unsigned long seqNumber);
int es10b_AuthenticateServer(struct euicc_ctx *ctx, char **b64_response, struct es10b_AuthenticateServer_param *param);
int es10b_CancelSession(struct euicc_ctx *ctx, const unsigned char *transactionId, unsigned char transactionIdLen, unsigned char reason);

void es10b_notification_metadata_free_all(struct es10b_notification_metadata *notifications);
void es10b_notification_free(struct es10b_notification *notification);
