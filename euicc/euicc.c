#include "euicc.private.h"
#include "hexutil.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define ISD_R_AID "\xA0\x00\x00\x05\x59\x10\x10\xFF\xFF\xFF\xFF\x89\x00\x00\x01\x00"

#define APDU_EUICC_HEADER 0x80, 0xE2
#define APDU_CONTINUE_READ_HEADER 0x80, 0xC0, 0x00, 0x00

static int es10x_transmit(struct euicc_ctx *ctx, struct apdu_response *response, struct apdu_request *req, unsigned req_len)
{
    req->cla = (req->cla & 0xF0) | (ctx->apdu._internal.logic_channel & 0x0F);
    return euicc_apdu_transmit(ctx, response, req, req_len);
}

static int es10x_transmit_iter(struct euicc_ctx *ctx, struct apdu_request *req, unsigned req_len, int (*callback)(struct apdu_response *response, void *userdata), void *userdata)
{
    struct apdu_request *request = NULL;
    struct apdu_response response;

    if (es10x_transmit(ctx, &response, req, req_len) < 0)
    {
        return -1;
    }

    do
    {
        if (response.length > 0)
        {
            if (callback(&response, userdata) < 0)
            {
                return -1;
            }
        }

        euicc_apdu_response_free(&response);

        if (response.sw1 == SW1_LAST)
        {
            int ret;

            if ((ret = euicc_apdu_le(ctx, &request, APDU_CONTINUE_READ_HEADER, response.sw2)) < 0)
            {
                return -1;
            }

            if (es10x_transmit(ctx, &response, request, ret) < 0)
            {
                return -1;
            }

            continue;
        }
        else if ((response.sw1 & 0xF0) == SW1_OK)
        {
            return 0;
        }

        return -1;
    } while (1);
}

int es10x_command_buildrequest(struct euicc_ctx *ctx, struct apdu_request **request, uint8_t p1, uint8_t p2, const uint8_t *der_req, unsigned req_len)
{
    int ret;

    ret = euicc_apdu_lc(ctx, request, APDU_EUICC_HEADER, p1, p2, req_len);
    if (ret < 0)
        return ret;

    memcpy((*request)->data, der_req, req_len);

    return ret;
}

static int es10x_command_buildrequest_continue(struct euicc_ctx *ctx, uint8_t reqseq, struct apdu_request **request, const uint8_t *der_req, unsigned req_len)
{
    return es10x_command_buildrequest(ctx, request, 0x11, reqseq, der_req, req_len);
}

static int es10x_command_buildrequest_last(struct euicc_ctx *ctx, uint8_t reqseq, struct apdu_request **request, const uint8_t *der_req, unsigned req_len)
{
    return es10x_command_buildrequest(ctx, request, 0x91, reqseq, der_req, req_len);
}

int es10x_command_iter(struct euicc_ctx *ctx, const uint8_t *der_req, unsigned req_len, int (*callback)(struct apdu_response *response, void *userdata), void *userdata)
{
    int ret, reqseq;
    struct apdu_request *req;
    const uint8_t *req_ptr;

    reqseq = 0;
    req_ptr = der_req;
    while (req_len)
    {
        uint8_t rlen;
        if (req_len > 120)
        {
            rlen = 120;
            ret = es10x_command_buildrequest_continue(ctx, reqseq, &req, req_ptr, rlen);
        }
        else
        {
            rlen = req_len;
            ret = es10x_command_buildrequest_last(ctx, reqseq, &req, req_ptr, rlen);
        }
        req_len -= rlen;

        if (ret < 0)
            return -1;

        ret = es10x_transmit_iter(ctx, req, ret, callback, userdata);
        if (ret < 0)
            return -1;

        req_ptr += rlen;
        reqseq++;
    }

    return 0;
}

struct userdata_es10x_command
{
    uint8_t *resp;
    unsigned resp_len;
};

static int iter_es10x_command(struct apdu_response *response, void *userdata)
{
    struct userdata_es10x_command *ud = (struct userdata_es10x_command *)userdata;
    uint8_t *new_response_data;

    new_response_data = realloc(ud->resp, ud->resp_len + response->length);
    if (!new_response_data)
    {
        return -1;
    }
    ud->resp = new_response_data;
    memcpy(ud->resp + ud->resp_len, response->data, response->length);
    ud->resp_len += response->length;
    return 0;
}

int es10x_command(struct euicc_ctx *ctx, uint8_t **resp, unsigned *resp_len, const uint8_t *der_req, unsigned req_len)
{
    int ret = 0;
    struct userdata_es10x_command ud;

    *resp = NULL;
    *resp_len = 0;
    memset(&ud, 0, sizeof(ud));

    ret = es10x_command_iter(ctx, der_req, req_len, iter_es10x_command, &ud);
    if (ret < 0)
    {
        free(ud.resp);
        return -1;
    }

    *resp = ud.resp;
    *resp_len = ud.resp_len;
    return 0;
}

int euicc_init(struct euicc_ctx *ctx)
{
    int ret;

    ret = ctx->apdu.interface->connect(ctx);
    if (ret < 0)
    {
        return -1;
    }

    ret = ctx->apdu.interface->logic_channel_open(ctx, (const uint8_t *)ISD_R_AID, sizeof(ISD_R_AID) - 1);
    if (ret < 0)
    {
        return -1;
    }

    ctx->apdu._internal.logic_channel = ret;

    return 0;
}

void euicc_fini(struct euicc_ctx *ctx)
{
    ctx->apdu.interface->logic_channel_close(ctx, ctx->apdu._internal.logic_channel);
    ctx->apdu.interface->disconnect(ctx);
    ctx->apdu._internal.logic_channel = 0;
}

void euicc_http_cleanup(struct euicc_ctx *ctx)
{
    free(ctx->http._internal.transaction_id_http);
    free(ctx->http._internal.transaction_id_bin);
    free(ctx->http._internal.b64_euicc_challenge);
    free(ctx->http._internal.b64_euicc_info_1);
    es10b_authenticate_server_param_free(ctx->http._internal.authenticate_server_param);
    free(ctx->http._internal.authenticate_server_param);
    free(ctx->http._internal.b64_authenticate_server_response);
    es10b_prepare_download_param_free(ctx->http._internal.prepare_download_param);
    free(ctx->http._internal.prepare_download_param);
    free(ctx->http._internal.b64_prepare_download_response);
    free(ctx->http._internal.b64_bound_profile_package);
    free(ctx->http._internal.b64_cancel_session_response);
    memset(&ctx->http._internal, 0, sizeof(ctx->http._internal));
}
