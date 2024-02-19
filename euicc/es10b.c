#include "es10b.h"
#include "es10x.private.h"

#include "derutils.h"
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

int es10b_PrepareDownload(struct euicc_ctx *ctx, char **b64_response, struct es10b_PrepareDownload_param *param)
{
    int fret = 0;
    uint8_t *reqbuf = NULL;
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    EUICC_SHA256_CTX sha256ctx;
    uint8_t hashCC[SHA256_BLOCK_SIZE];

    uint8_t *smdpSigned2 = NULL, *smdpSignature2 = NULL, *smdpCertificate = NULL;
    int smdpSigned2_len, smdpSignature2_len, smdpCertificate_len;
    struct derutils_node n_request, n_smdpSigned2, n_smdpSignature2, n_smdpCertificate, n_hashCc, n_transactionId, n_ccRequiredFlag;

    *b64_response = NULL;

    memset(&n_request, 0, sizeof(n_request));
    memset(&n_smdpSigned2, 0, sizeof(n_smdpSigned2));
    memset(&n_smdpSignature2, 0, sizeof(n_smdpSignature2));
    memset(&n_smdpCertificate, 0, sizeof(n_smdpCertificate));
    memset(&n_hashCc, 0, sizeof(n_hashCc));

    smdpSigned2 = malloc(euicc_base64_decode_len(param->b64_smdpSigned2));
    if (!smdpSigned2)
    {
        goto err;
    }

    smdpSignature2 = malloc(euicc_base64_decode_len(param->b64_smdpSignature2));
    if (!smdpSignature2)
    {
        goto err;
    }

    smdpCertificate = malloc(euicc_base64_decode_len(param->b64_smdpCertificate));
    if (!smdpCertificate)
    {
        goto err;
    }

    if ((smdpSigned2_len = euicc_base64_decode(smdpSigned2, param->b64_smdpSigned2)) < 0)
    {
        goto err;
    }

    if ((smdpSignature2_len = euicc_base64_decode(smdpSignature2, param->b64_smdpSignature2)) < 0)
    {
        goto err;
    }

    if ((smdpCertificate_len = euicc_base64_decode(smdpCertificate, param->b64_smdpCertificate)) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_smdpSigned2, 0x30, smdpSigned2, smdpSigned2_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_smdpSignature2, 0x5F37, smdpSignature2, smdpSignature2_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_smdpCertificate, 0x30, smdpCertificate, smdpCertificate_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_transactionId, 0x80, n_smdpSigned2.value, n_smdpSigned2.length) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_ccRequiredFlag, 0x01, n_smdpSigned2.value, n_smdpSigned2.length) < 0)
    {
        goto err;
    }

    n_request.tag = 0xBF21;
    n_request.pack.child = &n_smdpSigned2;
    n_smdpSigned2.pack.next = &n_smdpSignature2;

    if (derutils_convert_bin2long(n_ccRequiredFlag.value, n_ccRequiredFlag.length))
    {
        if (!param->str_confirmationCode || strlen(param->str_confirmationCode) == 0)
        {
            goto err;
        }

        memset(&sha256ctx, 0, sizeof(sha256ctx));
        euicc_sha256_init(&sha256ctx);
        euicc_sha256_update(&sha256ctx, (uint8_t *)param->str_confirmationCode, strlen(param->str_confirmationCode));
        euicc_sha256_final(&sha256ctx, hashCC);

        memset(&sha256ctx, 0, sizeof(sha256ctx));
        euicc_sha256_init(&sha256ctx);
        euicc_sha256_update(&sha256ctx, hashCC, sizeof(hashCC));
        euicc_sha256_update(&sha256ctx, n_transactionId.value, n_transactionId.length);
        euicc_sha256_final(&sha256ctx, hashCC);

        n_hashCc.tag = 0x04;
        n_hashCc.value = hashCC;
        n_hashCc.length = sizeof(hashCC);

        n_smdpSignature2.pack.next = &n_hashCc;
        n_hashCc.pack.next = &n_smdpCertificate;
    }
    else
    {
        n_smdpSignature2.pack.next = &n_smdpCertificate;
    }

    if (derutils_pack_alloc(&reqbuf, &reqlen, &n_request) < 0)
    {
        goto err;
    }

    free(smdpSigned2);
    smdpSigned2 = NULL;
    free(smdpSignature2);
    smdpSignature2 = NULL;
    free(smdpCertificate);
    smdpCertificate = NULL;

    if (es10x_command(ctx, &respbuf, &resplen, reqbuf, reqlen) < 0)
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

    if (euicc_base64_encode(*b64_response, respbuf, resplen) < 0)
    {
        goto err;
    }

    fret = 0;

    goto exit;

err:
    fret = -1;
    free(*b64_response);
    *b64_response = NULL;
exit:
    free(smdpSigned2);
    smdpSigned2 = NULL;
    free(smdpSignature2);
    smdpSignature2 = NULL;
    free(smdpCertificate);
    smdpCertificate = NULL;
    free(reqbuf);
    reqbuf = NULL;
    free(respbuf);
    respbuf = NULL;
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

int es10b_GetEUICCChallenge(struct euicc_ctx *ctx, char **b64_payload)
{
    int fret = 0;
    struct derutils_node n_request = {
        .tag = 0xBF2E, // GetEuiccDataRequest
    };
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct derutils_node tmpnode;

    reqlen = sizeof(ctx->apdu_request_buffer.body);
    if (derutils_pack(ctx->apdu_request_buffer.body, &reqlen, &n_request))
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu_request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, n_request.tag, respbuf, resplen))
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, 0x80, tmpnode.value, tmpnode.length))
    {
        goto err;
    }

    *b64_payload = malloc(euicc_base64_encode_len(tmpnode.length));
    if (!(*b64_payload))
    {
        goto err;
    }
    if (euicc_base64_encode(*b64_payload, tmpnode.value, tmpnode.length) < 0)
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
    respbuf = NULL;
    return fret;
}

int es10b_GetEUICCInfo(struct euicc_ctx *ctx, char **b64_payload)
{
    int fret = 0;
    struct derutils_node n_request = {
        .tag = 0xBF20, // GetEuiccInfo1Request
    };
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct derutils_node tmpnode;

    reqlen = sizeof(ctx->apdu_request_buffer.body);
    if (derutils_pack(ctx->apdu_request_buffer.body, &reqlen, &n_request))
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu_request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, n_request.tag, respbuf, resplen))
    {
        goto err;
    }

    *b64_payload = malloc(euicc_base64_encode_len(tmpnode.self.length));
    if (!(*b64_payload))
    {
        goto err;
    }
    if (euicc_base64_encode(*b64_payload, tmpnode.self.ptr, tmpnode.self.length) < 0)
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
    respbuf = NULL;
    return fret;
}

int es10b_ListNotification(struct euicc_ctx *ctx, struct es10b_notification_metadata **metadatas)
{
    int fret = 0;
    struct derutils_node n_request = {
        .tag = 0xBF28, // ListNotificationRequest
    };
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct derutils_node tmpnode, n_notificationMetadataList, n_NotificationMetadata;

    struct es10b_notification_metadata *metadatas_wptr;

    *metadatas = NULL;

    reqlen = sizeof(ctx->apdu_request_buffer.body);
    if (derutils_pack(ctx->apdu_request_buffer.body, &reqlen, &n_request))
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu_request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, n_request.tag, respbuf, resplen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_notificationMetadataList, 0xA0, tmpnode.value, tmpnode.length) < 0)
    {
        goto err;
    }

    n_NotificationMetadata.self.ptr = n_notificationMetadataList.value;
    n_NotificationMetadata.self.length = 0;

    while (derutils_unpack_next(&n_NotificationMetadata, &n_NotificationMetadata, n_notificationMetadataList.value, n_notificationMetadataList.length) == 0)
    {
        struct es10b_notification_metadata *p;

        if (n_NotificationMetadata.tag != 0xBF2F)
        {
            continue;
        }

        p = malloc(sizeof(struct es10b_notification_metadata));
        if (!p)
        {
            goto err;
        }

        memset(p, 0, sizeof(*p));

        tmpnode.self.ptr = n_NotificationMetadata.value;
        tmpnode.self.length = 0;
        while (derutils_unpack_next(&tmpnode, &tmpnode, n_NotificationMetadata.value, n_NotificationMetadata.length) == 0)
        {
            switch (tmpnode.tag)
            {
            case 0x80:
                p->seqNumber = derutils_convert_bin2long(tmpnode.value, tmpnode.length);
                break;
            case 0x81:
                if (tmpnode.length >= 2)
                {
                    switch (tmpnode.value[1])
                    {
                    case 0x80:
                        p->profileManagementOperation = "install";
                        break;
                    case 0x40:
                        p->profileManagementOperation = "enable";
                        break;
                    case 0x20:
                        p->profileManagementOperation = "disable";
                        break;
                    case 0x10:
                        p->profileManagementOperation = "delete";
                        break;
                    }
                }
                break;
            case 0x0C:
                p->notificationAddress = malloc(tmpnode.length + 1);
                if (p->notificationAddress)
                {
                    memcpy(p->notificationAddress, tmpnode.value, tmpnode.length);
                    p->notificationAddress[tmpnode.length] = '\0';
                }
                break;
            case 0x5A:
                p->iccid = malloc((tmpnode.length * 2) + 1);
                if (p->iccid)
                {
                    if (euicc_hexutil_bin2gsmbcd(p->iccid, (tmpnode.length * 2) + 1, tmpnode.value, tmpnode.length) < 0)
                    {
                        free(p->iccid);
                        p->iccid = NULL;
                    }
                }
                break;
            }
        }

        if (*metadatas == NULL)
        {
            *metadatas = p;
        }
        else
        {
            metadatas_wptr->next = p;
        }

        metadatas_wptr = p;
    }

    goto exit;

err:
    fret = -1;
    es10b_notification_metadata_free_all(*metadatas);
exit:
    free(respbuf);
    respbuf = NULL;
    return fret;
}

int es10b_RetrieveNotificationsList(struct euicc_ctx *ctx, struct es10b_notification *notification, unsigned long seqNumber)
{
    int fret = 0;
    uint8_t seqNumber_buf[sizeof(seqNumber)];
    uint32_t seqNumber_buf_len = sizeof(seqNumber_buf);
    struct derutils_node n_request;
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct derutils_node tmpnode, n_PendingNotification, n_NotificationMetadata;

    memset(notification, 0, sizeof(struct es10b_notification));

    if (derutils_convert_long2bin(seqNumber_buf, &seqNumber_buf_len, seqNumber) < 0)
    {
        goto err;
    }

    n_request = (struct derutils_node){
        .tag = 0xBF2B, // RetrieveNotificationsListRequest
        .pack = {
            .child = &(struct derutils_node){
                .tag = 0xA0, // searchCriteria
                .pack = {
                    .child = &(struct derutils_node){
                        .tag = 0x80, // seqNumber
                        .value = seqNumber_buf,
                        .length = seqNumber_buf_len,
                    },
                },
            },
        },
    };

    reqlen = sizeof(ctx->apdu_request_buffer.body);
    if (derutils_pack(ctx->apdu_request_buffer.body, &reqlen, &n_request))
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu_request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, n_request.tag, respbuf, resplen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, 0xA0, tmpnode.value, tmpnode.length) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_alias_tags(&n_PendingNotification, (uint16_t[]){0xBF37, 0x30}, 2, tmpnode.value, tmpnode.length) < 0)
    {
        goto err;
    }

    switch (n_PendingNotification.tag)
    {
    case 0xBF37: // profileInstallationResult
        if (derutils_unpack_find_tag(&tmpnode, 0xBF27, n_PendingNotification.value, n_PendingNotification.length) < 0)
        {
            goto err;
        }
        if (derutils_unpack_find_tag(&n_NotificationMetadata, 0xBF2F, tmpnode.value, tmpnode.length) < 0)
        {
            goto err;
        }
        break;
    case 0x30: // otherSignedNotification
        if (derutils_unpack_find_tag(&n_NotificationMetadata, 0xBF2F, n_PendingNotification.value, n_PendingNotification.length) < 0)
        {
            goto err;
        }
        break;
    }

    if (derutils_unpack_find_tag(&tmpnode, 0x0C, n_NotificationMetadata.value, n_NotificationMetadata.length) < 0)
    {
        goto err;
    }

    notification->notificationAddress = malloc(tmpnode.length + 1);
    if (!notification->notificationAddress)
    {
        goto err;
    }
    memcpy(notification->notificationAddress, tmpnode.value, tmpnode.length);
    notification->notificationAddress[tmpnode.length] = '\0';

    notification->b64_payload = malloc(euicc_base64_encode_len(n_PendingNotification.self.length));
    if (!notification->b64_payload)
    {
        goto err;
    }
    if (euicc_base64_encode(notification->b64_payload, n_PendingNotification.self.ptr, n_PendingNotification.self.length) < 0)
    {
        goto err;
    }

    fret = 0;

    goto exit;

err:
    fret = -1;
    es10b_notification_free(notification);
exit:
    free(respbuf);
    respbuf = NULL;
    return fret;
}

int es10b_RemoveNotificationFromList(struct euicc_ctx *ctx, unsigned long seqNumber)
{
    int fret = 0;
    uint8_t seqNumber_buf[sizeof(seqNumber)];
    uint32_t seqNumber_buf_len = sizeof(seqNumber_buf);
    struct derutils_node n_request;
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct derutils_node tmpnode;

    if (derutils_convert_long2bin(seqNumber_buf, &seqNumber_buf_len, seqNumber) < 0)
    {
        goto err;
    }

    n_request = (struct derutils_node){
        .tag = 0xBF30, // NotificationSentRequest
        .pack = {
            .child = &(struct derutils_node){
                .tag = 0x80, // seqNumber
                .value = seqNumber_buf,
                .length = seqNumber_buf_len,
            },
        },
    };

    reqlen = sizeof(ctx->apdu_request_buffer.body);
    if (derutils_pack(ctx->apdu_request_buffer.body, &reqlen, &n_request))
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu_request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, n_request.tag, respbuf, resplen) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&tmpnode, 0x80, tmpnode.value, tmpnode.length) < 0)
    {
        goto err;
    }

    fret = derutils_convert_bin2long(tmpnode.value, tmpnode.length);

    goto exit;

err:
    fret = -1;
exit:
    free(respbuf);
    respbuf = NULL;
    return fret;
}

int es10b_AuthenticateServer(struct euicc_ctx *ctx, char **b64_response, struct es10b_AuthenticateServer_param *param)
{
    int fret = 0;
    uint8_t *reqbuf = NULL;
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    uint8_t imei[8];
    int imei_len;
    uint8_t *serverSigned1 = NULL, *serverSignature1 = NULL, *euiccCiPKIdToBeUsed = NULL, *serverCertificate = NULL;
    int serverSigned1_len, serverSignature1_len, euiccCiPKIdToBeUsed_len, serverCertificate_len;
    struct derutils_node n_request, n_serverSigned1, n_serverSignature1, n_euiccCiPKIdToBeUsed, n_serverCertificate, n_CtxParams1, n_matchingId, n_deviceInfo, n_tac, n_deviceCapabilities, n_imei;

    *b64_response = NULL;

    memset(&n_request, 0, sizeof(n_request));
    memset(&n_serverSigned1, 0, sizeof(n_serverSigned1));
    memset(&n_serverSignature1, 0, sizeof(n_serverSignature1));
    memset(&n_euiccCiPKIdToBeUsed, 0, sizeof(n_euiccCiPKIdToBeUsed));
    memset(&n_serverCertificate, 0, sizeof(n_serverCertificate));
    memset(&n_CtxParams1, 0, sizeof(n_CtxParams1));
    memset(&n_matchingId, 0, sizeof(n_matchingId));
    memset(&n_deviceInfo, 0, sizeof(n_deviceInfo));
    memset(&n_tac, 0, sizeof(n_tac));
    memset(&n_deviceCapabilities, 0, sizeof(n_deviceCapabilities));
    memset(&n_imei, 0, sizeof(n_imei));

    if (param->imei)
    {
        if ((imei_len = euicc_hexutil_gsmbcd2bin(imei, sizeof(imei), param->imei)) < 0)
        {
            goto err;
        }
    }
    else
    {
        memcpy(imei, "\x35\x29\x06\x11", 4);
    }

    serverSigned1 = malloc(euicc_base64_decode_len(param->b64_serverSigned1));
    if (!serverSigned1)
    {
        goto err;
    }

    serverSignature1 = malloc(euicc_base64_decode_len(param->b64_serverSignature1));
    if (!serverSignature1)
    {
        goto err;
    }

    euiccCiPKIdToBeUsed = malloc(euicc_base64_decode_len(param->b64_euiccCiPKIdToBeUsed));
    if (!euiccCiPKIdToBeUsed)
    {
        goto err;
    }

    serverCertificate = malloc(euicc_base64_decode_len(param->b64_serverCertificate));
    if (!serverCertificate)
    {
        goto err;
    }

    if ((serverSigned1_len = euicc_base64_decode(serverSigned1, param->b64_serverSigned1)) < 0)
    {
        goto err;
    }

    if ((serverSignature1_len = euicc_base64_decode(serverSignature1, param->b64_serverSignature1)) < 0)
    {
        goto err;
    }

    if ((euiccCiPKIdToBeUsed_len = euicc_base64_decode(euiccCiPKIdToBeUsed, param->b64_euiccCiPKIdToBeUsed)) < 0)
    {
        goto err;
    }

    if ((serverCertificate_len = euicc_base64_decode(serverCertificate, param->b64_serverCertificate)) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_serverSigned1, 0x30, serverSigned1, serverSigned1_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_serverSignature1, 0x5F37, serverSignature1, serverSignature1_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_euiccCiPKIdToBeUsed, 0x04, euiccCiPKIdToBeUsed, euiccCiPKIdToBeUsed_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_serverCertificate, 0x30, serverCertificate, serverCertificate_len) < 0)
    {
        goto err;
    }

    n_request.tag = 0xBF38;
    n_request.pack.child = &n_serverSigned1;
    n_serverSigned1.pack.next = &n_serverSignature1;
    n_serverSignature1.pack.next = &n_euiccCiPKIdToBeUsed;
    n_euiccCiPKIdToBeUsed.pack.next = &n_serverCertificate;
    n_serverCertificate.pack.next = &n_CtxParams1;
    n_CtxParams1.tag = 0xA0;

    n_deviceInfo.tag = 0xA1;
    n_deviceInfo.pack.child = &n_tac;
    n_tac.tag = 0x80;
    n_tac.value = imei;
    n_tac.length = 4;
    n_tac.pack.next = &n_deviceCapabilities;
    n_deviceCapabilities.tag = 0xA1;
    if (param->imei)
    {
        n_deviceCapabilities.pack.next = &n_imei;
        n_imei.tag = 0x82;
        n_imei.value = imei;
        n_imei.length = imei_len;
    }

    if (param->matchingId)
    {
        n_CtxParams1.pack.child = &n_matchingId;
        n_matchingId.tag = 0x80;
        n_matchingId.value = (const uint8_t *)param->matchingId;
        n_matchingId.length = strlen(param->matchingId);
        n_matchingId.pack.next = &n_deviceInfo;
    }
    else
    {
        n_CtxParams1.pack.child = &n_deviceInfo;
    }

    if (derutils_pack_alloc(&reqbuf, &reqlen, &n_request) < 0)
    {
        goto err;
    }

    free(serverSigned1);
    serverSigned1 = NULL;
    free(serverSignature1);
    serverSignature1 = NULL;
    free(euiccCiPKIdToBeUsed);
    euiccCiPKIdToBeUsed = NULL;
    free(serverCertificate);
    serverCertificate = NULL;

    if (es10x_command(ctx, &respbuf, &resplen, reqbuf, reqlen) < 0)
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

    if (euicc_base64_encode(*b64_response, respbuf, resplen) < 0)
    {
        goto err;
    }

    fret = 0;

    goto exit;

err:
    fret = -1;
    free(*b64_response);
    *b64_response = NULL;
exit:
    free(serverSigned1);
    serverSigned1 = NULL;
    free(serverSignature1);
    serverSignature1 = NULL;
    free(euiccCiPKIdToBeUsed);
    euiccCiPKIdToBeUsed = NULL;
    free(serverCertificate);
    serverCertificate = NULL;
    free(reqbuf);
    reqbuf = NULL;
    free(respbuf);
    respbuf = NULL;
    return fret;
}

void es10b_notification_metadata_free_all(struct es10b_notification_metadata *metadatas)
{
    while (metadatas)
    {
        struct es10b_notification_metadata *next = metadatas->next;
        free(metadatas->notificationAddress);
        free(metadatas->iccid);
        free(metadatas);
        metadatas = next;
    }
}

void es10b_notification_free(struct es10b_notification *notification)
{
    free(notification->notificationAddress);
    free(notification->b64_payload);
    memset(notification, 0, sizeof(struct es10b_notification));
}
