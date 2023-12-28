#include "es10b.h"
#include "es10x.private.h"

#include "hexutil.h"
#include "derutil.h"
#include "base64.h"
#include "sha256.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "asn1c/asn1/BoundProfilePackage.h"
#include "asn1c/asn1/ProfileInstallationResult.h"
#include "asn1c/asn1/GetEuiccChallengeRequest.h"
#include "asn1c/asn1/GetEuiccChallengeResponse.h"
#include "asn1c/asn1/GetEuiccInfo1Request.h"
#include "asn1c/asn1/ListNotificationRequest.h"
#include "asn1c/asn1/ListNotificationResponse.h"
#include "asn1c/asn1/RetrieveNotificationsListRequest.h"
#include "asn1c/asn1/RetrieveNotificationsListResponse.h"
#include "asn1c/asn1/NotificationSentRequest.h"
#include "asn1c/asn1/NotificationSentResponse.h"
#include "asn1c/asn1/CtxParams1.h"

int es10b_prepare_download(struct euicc_ctx *ctx, char **b64_response, struct es10b_prepare_download_param *param)
{
    int fret = 0;
    uint8_t *reqbuf = NULL, *reqwptr = NULL;
    unsigned reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    *b64_response = NULL;

    reqlen = 0;
    reqlen += 7; // PrepareDownload Tag + Length (MAX)
    reqlen += euicc_base64_decode_len(param->b64_smdp_signed_2);
    reqlen += euicc_base64_decode_len(param->b64_smdp_signature_2);
    reqlen += SHA256_BLOCK_SIZE + 1 + 1; // hashCC + tag + len
    reqlen += euicc_base64_decode_len(param->b64_smdp_certificate);

    reqbuf = malloc(reqlen);
    if (!reqbuf)
    {
        goto err;
    }
    memset(reqbuf, 0, reqlen);

    reqwptr = reqbuf;
    reqwptr += 7; // Skip Tag + Length
    reqwptr += euicc_base64_decode(reqwptr, param->b64_smdp_signed_2);
    reqwptr += euicc_base64_decode(reqwptr, param->b64_smdp_signature_2);
    if (param->hexstr_transcation_id && param->str_checkcode)
    {
        EUICC_SHA256_CTX sha256ctx;
        uint8_t hashCC[SHA256_BLOCK_SIZE];
        uint8_t merged[sizeof(hashCC) + 16];
        int mergedlen;

        memset(&sha256ctx, 0, sizeof(sha256ctx));
        euicc_sha256_init(&sha256ctx);
        euicc_sha256_update(&sha256ctx, (uint8_t *)param->str_checkcode, strlen(param->str_checkcode));
        euicc_sha256_final(&sha256ctx, hashCC);

        memcpy(merged, hashCC, sizeof(hashCC));
        mergedlen = euicc_hexutil_hex2bin(merged + sizeof(hashCC), sizeof(merged) - sizeof(hashCC), param->hexstr_transcation_id);
        if (mergedlen < 0)
        {
            goto err;
        }
        mergedlen += sizeof(hashCC);

        memset(&sha256ctx, 0, sizeof(sha256ctx));
        euicc_sha256_init(&sha256ctx);
        euicc_sha256_update(&sha256ctx, merged, mergedlen);
        euicc_sha256_final(&sha256ctx, hashCC);

        *reqwptr++ = 0x04;
        *reqwptr++ = sizeof(hashCC);
        memcpy(reqwptr, hashCC, sizeof(hashCC));
        reqwptr += sizeof(hashCC);
    }
    reqwptr += euicc_base64_decode(reqwptr, param->b64_smdp_certificate);

    reqlen = reqwptr - reqbuf; // re-calculated length, for not accounting base64_decode_len

    reqwptr = euicc_derutil_tag_leftpad(reqbuf, reqlen, 7, 0xBF21);
    reqlen = reqlen - (reqwptr - reqbuf); // re-calculated length, for add tag and length

    if (es10x_command(ctx, &respbuf, &resplen, reqwptr, reqlen) < 0)
    {
        goto err;
    }
    free(reqbuf);
    reqbuf = NULL;

    *b64_response = malloc(euicc_base64_encode_len(resplen));
    if (!(*b64_response))
    {
        goto err;
    }
    euicc_base64_encode(*b64_response, respbuf, resplen);

    goto exit;

err:
    fret = -1;
    free(reqbuf);
exit:
    return fret;
}

int es10b_load_bound_profile_package(struct euicc_ctx *ctx, const char *b64_bpp)
{
    int fret = 0, ret;
    uint8_t *bpp_buf = NULL;
    int bpp_len;
    uint8_t prefix_len;
    asn_dec_rval_t asn1drval_bpp;
    BoundProfilePackage_t *bpp_asn1 = NULL;
    asn_enc_rval_t asn1erval;
    uint8_t *reqbuf = NULL;
    int reqlen;
    uint8_t *ptrtmp;
    struct
    {
        uint8_t *buf;
        int len;
    } apdus[64];
    int apdus_count = 0;
    ProfileInstallationResult_t *asn1resp = NULL;

    bpp_buf = malloc(euicc_base64_decode_len(b64_bpp));
    if (!bpp_buf)
    {
        goto err;
    }
    if ((bpp_len = euicc_base64_decode(bpp_buf, b64_bpp)) < 0)
    {
        goto err;
    }

    asn1drval_bpp = ber_decode(NULL, &asn_DEF_BoundProfilePackage, (void **)&bpp_asn1, bpp_buf, bpp_len);
    if (asn1drval_bpp.code != RC_OK)
    {
        goto err;
    }

    ret = euicc_derutil_tag_find(&ptrtmp, bpp_buf, bpp_len, NULL, 1);
    if (ret < 0)
    {
        goto err;
    }
    prefix_len = ptrtmp - bpp_buf;

    asn1erval = der_encode(&asn_DEF_InitialiseSecureChannelRequest, (void **)&bpp_asn1->initialiseSecureChannelRequest, NULL, NULL);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded + prefix_len;
    reqbuf = malloc(reqlen);
    if (!reqlen)
    {
        goto err;
    }
    memcpy(reqbuf, bpp_buf, prefix_len);
    asn1erval = der_encode_to_buffer(&asn_DEF_InitialiseSecureChannelRequest, &bpp_asn1->initialiseSecureChannelRequest, reqbuf + prefix_len, reqlen - prefix_len);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded + prefix_len;

    apdus[apdus_count].buf = reqbuf;
    apdus[apdus_count].len = reqlen;
    apdus_count++;
    reqbuf = NULL;

    asn1erval = der_encode(&asn_DEF_SeqBoundProfilePackageTLV87, &bpp_asn1->firstSequenceOf87, NULL, NULL);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded;
    reqbuf = malloc(reqlen);
    if (!reqlen)
    {
        goto err;
    }
    asn1erval = der_encode_to_buffer(&asn_DEF_SeqBoundProfilePackageTLV87, &bpp_asn1->firstSequenceOf87, reqbuf, reqlen);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded;

    apdus[apdus_count].buf = reqbuf;
    apdus[apdus_count].len = reqlen;
    apdus_count++;
    reqbuf = NULL;

    asn1erval = der_encode(&asn_DEF_SeqBoundProfilePackageTLV88, &bpp_asn1->sequenceOf88, NULL, NULL);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded;
    reqbuf = malloc(reqlen);
    if (!reqlen)
    {
        goto err;
    }
    asn1erval = der_encode_to_buffer(&asn_DEF_SeqBoundProfilePackageTLV88, &bpp_asn1->sequenceOf88, reqbuf, reqlen);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded;

    ret = euicc_derutil_tag_find(&ptrtmp, reqbuf, reqlen, NULL, 1);
    if (ret < 0)
    {
        goto err;
    }
    reqlen = ptrtmp - reqbuf;

    apdus[apdus_count].buf = reqbuf;
    apdus[apdus_count].len = reqlen;
    apdus_count++;
    reqbuf = NULL;

    for (int i = 0; i < bpp_asn1->sequenceOf88.list.count; i++)
    {
        asn1erval = der_encode(&asn_DEF_BoundProfilePackageTLV88, bpp_asn1->sequenceOf88.list.array[i], NULL, NULL);
        if (asn1erval.encoded == -1)
        {
            goto err;
        }
        reqlen = asn1erval.encoded;
        reqbuf = malloc(reqlen);
        if (!reqlen)
        {
            goto err;
        }
        asn1erval = der_encode_to_buffer(&asn_DEF_BoundProfilePackageTLV88, bpp_asn1->sequenceOf88.list.array[i], reqbuf, reqlen);
        if (asn1erval.encoded == -1)
        {
            goto err;
        }
        reqlen = asn1erval.encoded;

        apdus[apdus_count].buf = reqbuf;
        apdus[apdus_count].len = reqlen;
        apdus_count++;
        reqbuf = NULL;
    }

    if (bpp_asn1->secondSequenceOf87)
    {
        asn1erval = der_encode(&asn_DEF_SeqSecondBoundProfilePackageTLV87, bpp_asn1->secondSequenceOf87, NULL, NULL);
        if (asn1erval.encoded == -1)
        {
            goto err;
        }
        reqlen = asn1erval.encoded;
        reqbuf = malloc(reqlen);
        if (!reqlen)
        {
            goto err;
        }
        asn1erval = der_encode_to_buffer(&asn_DEF_SeqSecondBoundProfilePackageTLV87, bpp_asn1->secondSequenceOf87, reqbuf, reqlen);
        if (asn1erval.encoded == -1)
        {
            goto err;
        }
        reqlen = asn1erval.encoded;

        apdus[apdus_count].buf = reqbuf;
        apdus[apdus_count].len = reqlen;
        apdus_count++;
        reqbuf = NULL;
    }

    asn1erval = der_encode(&asn_DEF_SeqBoundProfilePackageTLV86, &bpp_asn1->sequenceOf86, NULL, NULL);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded;
    reqbuf = malloc(reqlen);
    if (!reqlen)
    {
        goto err;
    }
    asn1erval = der_encode_to_buffer(&asn_DEF_SeqBoundProfilePackageTLV86, &bpp_asn1->sequenceOf86, reqbuf, reqlen);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    reqlen = asn1erval.encoded;

    ret = euicc_derutil_tag_find(&ptrtmp, reqbuf, reqlen, NULL, 1);
    if (ret < 0)
    {
        goto err;
    }
    reqlen = ptrtmp - reqbuf;

    apdus[apdus_count].buf = reqbuf;
    apdus[apdus_count].len = reqlen;
    apdus_count++;
    reqbuf = NULL;

    for (int i = 0; i < bpp_asn1->sequenceOf86.list.count; i++)
    {
        asn1erval = der_encode(&asn_DEF_BoundProfilePackageTLV86, bpp_asn1->sequenceOf86.list.array[i], NULL, NULL);
        if (asn1erval.encoded == -1)
        {
            goto err;
        }
        reqlen = asn1erval.encoded;
        reqbuf = malloc(reqlen);
        if (!reqlen)
        {
            goto err;
        }
        asn1erval = der_encode_to_buffer(&asn_DEF_BoundProfilePackageTLV86, bpp_asn1->sequenceOf86.list.array[i], reqbuf, reqlen);
        if (asn1erval.encoded == -1)
        {
            goto err;
        }
        reqlen = asn1erval.encoded;

        apdus[apdus_count].buf = reqbuf;
        apdus[apdus_count].len = reqlen;
        apdus_count++;
        reqbuf = NULL;
    }

    // for (int i = 0; i < apdus_count; i++)
    // {
    //     printf("APDU[%d]: ", i);
    //     for (int j = 0; j < apdus[i].len; j++)
    //     {
    //         printf("%02X", apdus[i].buf[j]);
    //     }
    //     printf("\n");
    // }

    for (int i = 0; i < apdus_count; i++)
    {
        uint8_t *respbuf = NULL;
        unsigned resplen = 0;
        asn_dec_rval_t asn1drval;

        ret = es10x_command(ctx, &respbuf, &resplen, apdus[i].buf, apdus[i].len);
        if (ret < 0)
        {
            goto err;
        }
        if (resplen > 0)
        {
            asn1drval = ber_decode(NULL, &asn_DEF_ProfileInstallationResult, (void **)&asn1resp, respbuf, resplen);
            free(respbuf);
            respbuf = NULL;
            resplen = 0;
            if (asn1drval.code != RC_OK)
            {
                goto err;
            }
            if (asn1resp->profileInstallationResultData.finalResult.present != ProfileInstallationResultData__finalResult_PR_successResult)
            {
                goto err;
            }
            break;
        }
        free(respbuf);
        respbuf = NULL;
        resplen = 0;
    }

    goto exit;

err:
    fret = -1;
exit:
    free(bpp_buf);
    free(reqbuf);
    for (int i = 0; i < apdus_count; i++)
    {
        free(apdus[i].buf);
    }
    ASN_STRUCT_FREE(asn_DEF_BoundProfilePackage, bpp_asn1);
    ASN_STRUCT_FREE(asn_DEF_ProfileInstallationResult, asn1resp);
    return fret;
}

int es10b_get_euicc_challenge(struct euicc_ctx *ctx, char **b64_payload)
{
    int fret = 0;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    asn_dec_rval_t asn1drval;
    GetEuiccChallengeRequest_t *asn1req = NULL;
    GetEuiccChallengeResponse_t *asn1resp = NULL;

    asn1req = malloc(sizeof(GetEuiccChallengeRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1erval = der_encode_to_buffer(&asn_DEF_GetEuiccChallengeRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_GetEuiccChallengeRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->g_asn1_der_request_buf, asn1erval.encoded) < 0)
    {
        goto err;
    }

    asn1drval = ber_decode(NULL, &asn_DEF_GetEuiccChallengeResponse, (void **)&asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    *b64_payload = malloc(euicc_base64_encode_len(asn1resp->euiccChallenge.size));
    if (!(*b64_payload))
    {
        goto err;
    }
    if (euicc_base64_encode(*b64_payload, asn1resp->euiccChallenge.buf, asn1resp->euiccChallenge.size) < 0)
    {
        goto err;
    }

    goto exit;

err:
    fret = -1;
    free(*b64_payload);
    *b64_payload = NULL;
exit:
    free(respbuf);
    ASN_STRUCT_FREE(asn_DEF_GetEuiccChallengeRequest, asn1req);
    ASN_STRUCT_FREE(asn_DEF_GetEuiccChallengeResponse, asn1resp);

    return fret;
}

int es10b_get_euicc_info(struct euicc_ctx *ctx, char **b64_payload)
{
    int fret = 0;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    GetEuiccInfo1Request_t *asn1req = NULL;

    *b64_payload = NULL;

    asn1req = malloc(sizeof(GetEuiccInfo1Request_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1erval = der_encode_to_buffer(&asn_DEF_GetEuiccInfo1Request, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_GetEuiccInfo1Request, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->g_asn1_der_request_buf, asn1erval.encoded) < 0)
    {
        goto err;
    }

    *b64_payload = malloc(euicc_base64_encode_len(resplen));
    if (!(*b64_payload))
    {
        goto err;
    }
    if (euicc_base64_encode(*b64_payload, respbuf, resplen) < 0)
    {
        goto err;
    }

    goto exit;

err:
    fret = -1;
    free(*b64_payload);
    *b64_payload = NULL;
exit:
    free(respbuf);
    ASN_STRUCT_FREE(asn_DEF_GetEuiccInfo1Request, asn1req);

    return fret;
}

int es10b_list_notification(struct euicc_ctx *ctx, struct es10b_notification_metadata **metadatas, int *metadatas_count)
{
    int fret = 0;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    asn_dec_rval_t asn1drval;
    ListNotificationRequest_t *asn1req = NULL;
    ListNotificationResponse_t *asn1resp = NULL;

    *metadatas = NULL;
    *metadatas_count = 0;

    asn1req = malloc(sizeof(ListNotificationRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1erval = der_encode_to_buffer(&asn_DEF_ListNotificationRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_ListNotificationRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->g_asn1_der_request_buf, asn1erval.encoded) < 0)
    {
        goto err;
    }

    asn1drval = ber_decode(NULL, &asn_DEF_ListNotificationResponse, (void **)&asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    if (asn1resp->present != ListNotificationResponse_PR_notificationMetadataList)
    {
        goto err;
    }

    *metadatas_count = asn1resp->choice.notificationMetadataList.list.count;
    *metadatas = malloc(sizeof(struct es10b_notification_metadata) * (*metadatas_count));
    if (!(*metadatas))
    {
        goto err;
    }

    for (int i = 0; i < *metadatas_count; i++)
    {
        struct NotificationMetadata *asn1metadata = asn1resp->choice.notificationMetadataList.list.array[i];
        struct es10b_notification_metadata *metadata = &((*metadatas)[i]);

        memset(metadata, 0, sizeof(*metadata));
        asn_INTEGER2ulong(&asn1metadata->seqNumber, &metadata->seqNumber);

        switch (asn1metadata->profileManagementOperation.buf[0])
        {
        case 128:
            metadata->profileManagementOperation = strdup("install");
            break;
        case 64:
            metadata->profileManagementOperation = strdup("enable");
            break;
        case 32:
            metadata->profileManagementOperation = strdup("disable");
            break;
        case 16:
            metadata->profileManagementOperation = strdup("delete");
            break;
        }

        metadata->notificationAddress = malloc(asn1metadata->notificationAddress.size + 1);
        if (metadata->notificationAddress)
        {
            memcpy(metadata->notificationAddress, asn1metadata->notificationAddress.buf, asn1metadata->notificationAddress.size);
            metadata->notificationAddress[asn1metadata->notificationAddress.size] = '\0';
        }
        else
        {
            metadata->notificationAddress = NULL;
        }

        if (asn1metadata->iccid)
        {
            metadata->iccid = malloc((asn1metadata->iccid->size * 2) + 1);
            if (metadata->iccid)
            {
                if (euicc_hexutil_bin2gsmbcd(metadata->iccid, (asn1metadata->iccid->size * 2) + 1, asn1metadata->iccid->buf, asn1metadata->iccid->size) < 0)
                {
                    free(metadata->iccid);
                    metadata->iccid = NULL;
                }
            }
        }
        else
        {
            metadata->iccid = NULL;
        }
    }

    goto exit;

err:
    fret = -1;
    free(*metadatas);
    *metadatas = NULL;
    *metadatas_count = 0;
exit:
    free(respbuf);
    ASN_STRUCT_FREE(asn_DEF_ListNotificationRequest, asn1req);
    ASN_STRUCT_FREE(asn_DEF_ListNotificationResponse, asn1resp);

    return fret;
}

int es10b_retrieve_notification(struct euicc_ctx *ctx, char **b64_payload, char **receiver, unsigned long seqNumber)
{
    int fret = 0;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    asn_dec_rval_t asn1drval;
    RetrieveNotificationsListRequest_t *asn1req = NULL;
    RetrieveNotificationsListResponse_t *asn1resp = NULL;
    PendingNotification_t *asn1notification;
    NotificationMetadata_t *asn1metadata;
    uint8_t *payload = NULL;
    unsigned payload_length = 0;

    *b64_payload = NULL;
    *receiver = NULL;

    asn1req = malloc(sizeof(RetrieveNotificationsListRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    asn1req->searchCriteria = malloc(sizeof(struct RetrieveNotificationsListRequest__searchCriteria));
    if (!asn1req->searchCriteria)
    {
        goto err;
    }
    memset(asn1req->searchCriteria, 0, sizeof(*asn1req->searchCriteria));

    asn1req->searchCriteria->present = RetrieveNotificationsListRequest__searchCriteria_PR_seqNumber;
    if (asn_ulong2INTEGER(&asn1req->searchCriteria->choice.seqNumber, seqNumber) < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_RetrieveNotificationsListRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_RetrieveNotificationsListRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->g_asn1_der_request_buf, asn1erval.encoded) < 0)
    {
        goto err;
    }

    asn1drval = ber_decode(NULL, &asn_DEF_RetrieveNotificationsListResponse, (void **)&asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    if (asn1resp->present != RetrieveNotificationsListResponse_PR_notificationList)
    {
        goto err;
    }

    if (asn1resp->choice.notificationList.list.count < 1)
    {
        goto err;
    }

    asn1notification = asn1resp->choice.notificationList.list.array[0];

    switch (asn1notification->present)
    {
    case PendingNotification_PR_profileInstallationResult:
        asn1metadata = &asn1notification->choice.profileInstallationResult.profileInstallationResultData.notificationMetadata;
        break;
    case PendingNotification_PR_otherSignedNotification:
        asn1metadata = &asn1notification->choice.otherSignedNotification.tbsOtherNotification;
        break;
    default:
        goto err;
    }

    *receiver = malloc(asn1metadata->notificationAddress.size + 1);
    memset(*receiver, 0, asn1metadata->notificationAddress.size + 1);
    if (!(*receiver))
    {
        goto err;
    }
    memcpy(*receiver, asn1metadata->notificationAddress.buf, asn1metadata->notificationAddress.size);

    asn1erval = der_encode(&asn_DEF_PendingNotification, asn1notification, NULL, NULL);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    payload_length = asn1erval.encoded;
    payload = malloc(payload_length);
    if (!payload)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_PendingNotification, asn1notification, payload, payload_length);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    *b64_payload = malloc(euicc_base64_encode_len(payload_length));
    if (!(*b64_payload))
    {
        goto err;
    }
    if (euicc_base64_encode(*b64_payload, payload, payload_length) < 0)
    {
        goto err;
    }

    goto exit;

err:
    fret = -1;
    free(*b64_payload);
    *b64_payload = NULL;
    free(*receiver);
    *receiver = NULL;
exit:
    free(payload);
    free(respbuf);
    ASN_STRUCT_FREE(asn_DEF_RetrieveNotificationsListRequest, asn1req);
    ASN_STRUCT_FREE(asn_DEF_RetrieveNotificationsListResponse, asn1resp);

    return fret;
}

int es10b_remove_notification_from_list(struct euicc_ctx *ctx, unsigned long seqNumber)
{
    int fret = 0;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    asn_dec_rval_t asn1drval;
    NotificationSentRequest_t *asn1req = NULL;
    NotificationSentResponse_t *asn1resp = NULL;
    long eresult;

    asn1req = malloc(sizeof(NotificationSentRequest_t));
    if (!asn1req)
    {
        goto err;
    }
    memset(asn1req, 0, sizeof(*asn1req));

    if (asn_ulong2INTEGER(&asn1req->seqNumber, seqNumber) < 0)
    {
        goto err;
    }

    asn1erval = der_encode_to_buffer(&asn_DEF_NotificationSentRequest, asn1req, ctx->g_asn1_der_request_buf, sizeof(ctx->g_asn1_der_request_buf));
    ASN_STRUCT_FREE(asn_DEF_NotificationSentRequest, asn1req);
    asn1req = NULL;
    if (asn1erval.encoded == -1)
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->g_asn1_der_request_buf, asn1erval.encoded) < 0)
    {
        goto err;
    }

    asn1drval = ber_decode(NULL, &asn_DEF_NotificationSentResponse, (void **)&asn1resp, respbuf, resplen);
    free(respbuf);
    respbuf = NULL;

    if (asn1drval.code != RC_OK)
    {
        goto err;
    }

    if (asn_INTEGER2long(&asn1resp->deleteNotificationStatus, &eresult) < 0)
    {
        goto err;
    }

    fret = eresult;
    goto exit;

err:
    fret = -1;
exit:
    ASN_STRUCT_FREE(asn_DEF_NotificationSentRequest, asn1req);
    ASN_STRUCT_FREE(asn_DEF_NotificationSentResponse, asn1resp);
    return fret;
}

int es10b_authenticate_server(struct euicc_ctx *ctx, char **b64_response, struct es10b_authenticate_server_param *param)
{
    int fret = 0;
    uint8_t binimei[8];
    uint8_t binimei_len;
    uint8_t *reqbuf = NULL, *reqwptr = NULL;
    unsigned reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;
    asn_enc_rval_t asn1erval;
    CtxParams1_t *ctx_params1;

    *b64_response = NULL;

    ctx_params1 = malloc(sizeof(CtxParams1_t));
    if (!ctx_params1)
    {
        goto err;
    }
    memset(ctx_params1, 0, sizeof(*ctx_params1));

    ctx_params1->present = CtxParams1_PR_ctxParamsForCommonAuthentication;
    if (param->matchingId)
    {
        ctx_params1->choice.ctxParamsForCommonAuthentication.matchingId = malloc(sizeof(UTF8String_t));
        if (!ctx_params1->choice.ctxParamsForCommonAuthentication.matchingId)
        {
            goto err;
        }
        memset(ctx_params1->choice.ctxParamsForCommonAuthentication.matchingId, 0, sizeof(UTF8String_t));
        if (OCTET_STRING_fromString(ctx_params1->choice.ctxParamsForCommonAuthentication.matchingId, param->matchingId) < 0)
        {
            goto err;
        }
    }
    if (param->imei)
    {
        if ((binimei_len = euicc_hexutil_gsmbcd2bin(binimei, sizeof(binimei), param->imei)) < 0)
        {
            goto err;
        }
        ctx_params1->choice.ctxParamsForCommonAuthentication.deviceInfo.imei = malloc(sizeof(Octet8_t));
        if (!ctx_params1->choice.ctxParamsForCommonAuthentication.deviceInfo.imei)
        {
            goto err;
        }
        memset(ctx_params1->choice.ctxParamsForCommonAuthentication.deviceInfo.imei, 0, sizeof(Octet8_t));
        if (OCTET_STRING_fromBuf(ctx_params1->choice.ctxParamsForCommonAuthentication.deviceInfo.imei, (const char *)binimei, binimei_len) < 0)
        {
            goto err;
        }
        param->tac = binimei;
    }

    if (!param->tac)
    {
        param->tac = "\x35\x29\x06\x11";
    }
    if (OCTET_STRING_fromBuf(&ctx_params1->choice.ctxParamsForCommonAuthentication.deviceInfo.tac, (const char *)param->tac, 4) < 0)
    {
        goto err;
    }

    asn1erval = der_encode(&asn_DEF_CtxParams1, ctx_params1, NULL, NULL);

    reqlen = 0;
    reqlen += 7; // AuthenticateServerRequest Tag + Length (MAX)
    reqlen += euicc_base64_decode_len(param->b64_server_signed_1);
    reqlen += euicc_base64_decode_len(param->b64_server_signature_1);
    reqlen += euicc_base64_decode_len(param->b64_euicc_ci_pkid_to_be_used);
    reqlen += euicc_base64_decode_len(param->b64_server_certificate);
    reqlen += asn1erval.encoded;

    reqbuf = malloc(reqlen);
    if (!reqbuf)
    {
        goto err;
    }
    memset(reqbuf, 0, reqlen);

    reqwptr = reqbuf;
    reqwptr += 7; // Skip Tag + Length
    reqwptr += euicc_base64_decode(reqwptr, param->b64_server_signed_1);
    reqwptr += euicc_base64_decode(reqwptr, param->b64_server_signature_1);
    reqwptr += euicc_base64_decode(reqwptr, param->b64_euicc_ci_pkid_to_be_used);
    reqwptr += euicc_base64_decode(reqwptr, param->b64_server_certificate);
    asn1erval = der_encode_to_buffer(&asn_DEF_CtxParams1, ctx_params1, reqwptr, asn1erval.encoded);
    if (asn1erval.encoded == -1)
    {
        goto err;
    }
    ASN_STRUCT_FREE(asn_DEF_CtxParams1, ctx_params1);
    ctx_params1 = NULL;
    reqlen = reqwptr - reqbuf + asn1erval.encoded; // re-calculated length, for not accounting base64_decode_len

    reqwptr = euicc_derutil_tag_leftpad(reqbuf, reqlen, 7, 0xBF38);
    reqlen = reqlen - (reqwptr - reqbuf); // re-calculated length, for add tag and length

    if (es10x_command(ctx, &respbuf, &resplen, reqwptr, reqlen) < 0)
    {
        goto err;
    }
    free(reqbuf);
    reqbuf = NULL;

    *b64_response = malloc(euicc_base64_encode_len(resplen));
    if (!(*b64_response))
    {
        goto err;
    }
    euicc_base64_encode(*b64_response, respbuf, resplen);

    goto exit;

err:
    fret = -1;
    free(reqbuf);
exit:
    ASN_STRUCT_FREE(asn_DEF_CtxParams1, ctx_params1);
    return fret;
}

void es10b_notification_metadata_free_all(struct es10b_notification_metadata *metadatas, int count)
{
    if (!metadatas)
    {
        return;
    }
    for (int i = 0; i < count; i++)
    {
        free(metadatas[i].profileManagementOperation);
        free(metadatas[i].notificationAddress);
        free(metadatas[i].iccid);
    }
    free(metadatas);
}

void es10b_notification_metadata_print(struct es10b_notification_metadata *n)
{
    printf("\tseqNumber: %ld\n", n->seqNumber);
    printf("\tprofileManagementOperation: %s\n", n->profileManagementOperation);
    printf("\tnotificationAddress: %s\n", n->notificationAddress ? n->notificationAddress : "(null)");
    printf("\ticcid: %s\n", n->iccid ? n->iccid : "(null)");
}
