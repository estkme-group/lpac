#pragma once

#include "euicc.h"

struct es10b_PrepareDownload_param
{
    const char *b64_profileMetadata;
    const char *b64_smdpSigned2;
    const char *b64_smdpSignature2;
    const char *b64_smdpCertificate;
    const char *confirmationCode;
};

struct es10b_NotificationMetadataList
{
    unsigned long seqNumber;
    const char *profileManagementOperation;
    char *notificationAddress;
    char *iccid;

    struct es10b_NotificationMetadataList *next;
};

struct es10b_PendingNotification
{
    char *notificationAddress;
    char *b64_PendingNotification;
};

struct es10b_AuthenticateServer_param
{
    const char *b64_serverSigned1;
    const char *b64_serverSignature1;
    const char *b64_euiccCiPKIdToBeUsed;
    const char *b64_serverCertificate;
    const char *matchingId;
    const char *imei;
};

struct es10b_CancelSession_param
{
    const unsigned char *transactionId;
    unsigned char transactionIdLen;
    enum es10b_CancelSessionReason
    {
        ES10B_CANCELSESSIONREASON_ENDUSERREJECTION = 0,
        ES10B_CANCELSESSIONREASON_POSTPONED = 1,
        ES10B_CANCELSESSIONREASON_TIMEOUT = 2,
        ES10B_CANCELSESSIONREASON_PPRNOTALLOWED = 3,
        ES10B_CANCELSESSIONREASON_METADATAMISMATCH = 4,
        ES10B_CANCELSESSIONREASON_LOADBPPEXECUTIONERROR = 5,
        ES10B_CANCELSESSIONREASON_UNDEFINED = 127
    } reason;
};

int es10b_PrepareDownload(struct euicc_ctx *ctx, char **b64_PrepareDownloadResponse, struct es10b_PrepareDownload_param *param);
int es10b_LoadBoundProfilePackage(struct euicc_ctx *ctx, const char *b64_BoundProfilePackage);
int es10b_GetEUICCChallenge(struct euicc_ctx *ctx, char **b64_euiccChallenge);
int es10b_GetEUICCInfo(struct euicc_ctx *ctx, char **b64_EUICCInfo1);
int es10b_ListNotification(struct euicc_ctx *ctx, struct es10b_NotificationMetadataList **notificationMetadataList);
int es10b_RetrieveNotificationsList(struct euicc_ctx *ctx, struct es10b_PendingNotification *PendingNotification, unsigned long seqNumber);
int es10b_RemoveNotificationFromList(struct euicc_ctx *ctx, unsigned long seqNumber);
int es10b_AuthenticateServer(struct euicc_ctx *ctx, char **b64_AuthenticateServerResponse, struct es10b_AuthenticateServer_param *param);
int es10b_CancelSession(struct euicc_ctx *ctx, char **b64_CancelSessionResponse, struct es10b_CancelSession_param *param);

void es10b_notification_metadata_free_all(struct es10b_NotificationMetadataList *notificationMetadataList);
void es10b_notification_free(struct es10b_PendingNotification *PendingNotification);
