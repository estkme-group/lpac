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

int es10b_get_euicc_info(struct euicc_ctx *ctx, char **b64_payload)
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

int es10b_list_notification(struct euicc_ctx *ctx, struct es10b_notification_metadata **metadatas)
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

int es10b_retrieve_notification(struct euicc_ctx *ctx, struct es10b_notification *notification, unsigned long seqNumber)
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

    notification->receiver = malloc(tmpnode.length + 1);
    if (!notification->receiver)
    {
        goto err;
    }
    memcpy(notification->receiver, tmpnode.value, tmpnode.length);
    notification->receiver[tmpnode.length] = '\0';

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

int es10b_remove_notification_from_list(struct euicc_ctx *ctx, unsigned long seqNumber)
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

int es10b_authenticate_server(struct euicc_ctx *ctx, char **b64_response, struct es10b_authenticate_server_param *param)
{
    int fret = 0;
    uint8_t *reqbuf = NULL;
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    uint8_t imei[8];
    int imei_len;
    uint8_t *server_signed_1 = NULL, *server_signature_1 = NULL, *euicc_ci_pkid_to_be_used = NULL, *server_certificate = NULL;
    int server_signed_1_len, server_signature_1_len, euicc_ci_pkid_to_be_used_len, server_certificate_len;
    struct derutils_node n_request, n_server_signed_1, n_server_signature_1, n_euicc_ci_pkid_to_be_used, n_server_certificate, n_ctx_params1, n_matchingId, n_deviceInfo, n_tac, n_deviceCapabilities, n_imei;

    *b64_response = NULL;

    memset(&n_request, 0, sizeof(n_request));
    memset(&n_server_signed_1, 0, sizeof(n_server_signed_1));
    memset(&n_server_signature_1, 0, sizeof(n_server_signature_1));
    memset(&n_euicc_ci_pkid_to_be_used, 0, sizeof(n_euicc_ci_pkid_to_be_used));
    memset(&n_server_certificate, 0, sizeof(n_server_certificate));
    memset(&n_ctx_params1, 0, sizeof(n_ctx_params1));
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

    server_signed_1 = malloc(euicc_base64_decode_len(param->b64_server_signed_1));
    if (!server_signed_1)
    {
        goto err;
    }

    server_signature_1 = malloc(euicc_base64_decode_len(param->b64_server_signature_1));
    if (!server_signature_1)
    {
        goto err;
    }

    euicc_ci_pkid_to_be_used = malloc(euicc_base64_decode_len(param->b64_euicc_ci_pkid_to_be_used));
    if (!euicc_ci_pkid_to_be_used)
    {
        goto err;
    }

    server_certificate = malloc(euicc_base64_decode_len(param->b64_server_certificate));
    if (!server_certificate)
    {
        goto err;
    }

    if ((server_signed_1_len = euicc_base64_decode(server_signed_1, param->b64_server_signed_1)) < 0)
    {
        goto err;
    }

    if ((server_signature_1_len = euicc_base64_decode(server_signature_1, param->b64_server_signature_1)) < 0)
    {
        goto err;
    }

    if ((euicc_ci_pkid_to_be_used_len = euicc_base64_decode(euicc_ci_pkid_to_be_used, param->b64_euicc_ci_pkid_to_be_used)) < 0)
    {
        goto err;
    }

    if ((server_certificate_len = euicc_base64_decode(server_certificate, param->b64_server_certificate)) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_server_signed_1, 0x30, server_signed_1, server_signed_1_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_server_signature_1, 0x5F37, server_signature_1, server_signature_1_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_euicc_ci_pkid_to_be_used, 0x04, euicc_ci_pkid_to_be_used, euicc_ci_pkid_to_be_used_len) < 0)
    {
        goto err;
    }

    if (derutils_unpack_find_tag(&n_server_certificate, 0x30, server_certificate, server_certificate_len) < 0)
    {
        goto err;
    }

    n_request.tag = 0xBF38;
    n_request.pack.child = &n_server_signed_1;
    n_server_signed_1.pack.next = &n_server_signature_1;
    n_server_signature_1.pack.next = &n_euicc_ci_pkid_to_be_used;
    n_euicc_ci_pkid_to_be_used.pack.next = &n_server_certificate;
    n_server_certificate.pack.next = &n_ctx_params1;
    n_ctx_params1.tag = 0xA0;

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
        n_ctx_params1.pack.child = &n_matchingId;
        n_matchingId.tag = 0x80;
        n_matchingId.value = (const uint8_t *)param->matchingId;
        n_matchingId.length = strlen(param->matchingId);
        n_matchingId.pack.next = &n_deviceInfo;
    }
    else
    {
        n_ctx_params1.pack.child = &n_deviceInfo;
    }

    if (derutils_pack_alloc(&reqbuf, &reqlen, &n_request) < 0)
    {
        goto err;
    }

    free(server_signed_1);
    server_signed_1 = NULL;
    free(server_signature_1);
    server_signature_1 = NULL;
    free(euicc_ci_pkid_to_be_used);
    euicc_ci_pkid_to_be_used = NULL;
    free(server_certificate);
    server_certificate = NULL;

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
    free(server_signed_1);
    server_signed_1 = NULL;
    free(server_signature_1);
    server_signature_1 = NULL;
    free(euicc_ci_pkid_to_be_used);
    euicc_ci_pkid_to_be_used = NULL;
    free(server_certificate);
    server_certificate = NULL;
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
    free(notification->receiver);
    free(notification->b64_payload);
    memset(notification, 0, sizeof(struct es10b_notification));
}
