#include "euicc.private.h"
#include "es10a.h"

#include "hexutil.h"
#include "derutil.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int es10a_get_euicc_configured_addresses(struct euicc_ctx *ctx, struct es10a_euicc_configured_addresses *address)
{
    int fret = 0;
    struct euicc_derutil_node n_request = {
        .tag = 0xBF3C, // EuiccConfiguredAddressesRequest
    };
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct euicc_derutil_node tmpnode, n_Response;

    memset(address, 0, sizeof(*address));

    reqlen = sizeof(ctx->apdu._internal.request_buffer.body);
    if (euicc_derutil_pack(ctx->apdu._internal.request_buffer.body, &reqlen, &n_request))
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu._internal.request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&n_Response, n_request.tag, respbuf, resplen))
    {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&tmpnode, 0x80, n_Response.value, n_Response.length) == 0)
    {
        address->defaultDpAddress = malloc(tmpnode.length + 1);
        if (address->defaultDpAddress)
        {
            memcpy(address->defaultDpAddress, tmpnode.value, tmpnode.length);
            address->defaultDpAddress[tmpnode.length] = '\0';
        }
    }

    if (euicc_derutil_unpack_find_tag(&tmpnode, 0x81, n_Response.value, n_Response.length) == 0)
    {
        address->rootDsAddress = malloc(tmpnode.length + 1);
        if (address->rootDsAddress)
        {
            memcpy(address->rootDsAddress, tmpnode.value, tmpnode.length);
            address->rootDsAddress[tmpnode.length] = '\0';
        }
    }

    goto exit;

err:
    fret = -1;
    free(address->defaultDpAddress);
    address->defaultDpAddress = NULL;
    free(address->rootDsAddress);
    address->rootDsAddress = NULL;
exit:
    free(respbuf);
    respbuf = NULL;
    return fret;
}

int es10a_set_default_dp_address(struct euicc_ctx *ctx, const char *smdp)
{
    int fret = 0;
    struct euicc_derutil_node n_request = {
        .tag = 0xBF3F, // SetDefaultDpAddressRequest
        .pack = {
            .child = &(struct euicc_derutil_node){
                .tag = 0x80,
                .length = strlen(smdp),
                .value = (const uint8_t *)smdp,
            },
        },
    };
    uint32_t reqlen;
    uint8_t *respbuf = NULL;
    unsigned resplen;

    struct euicc_derutil_node tmpnode;

    reqlen = sizeof(ctx->apdu._internal.request_buffer.body);
    if (euicc_derutil_pack(ctx->apdu._internal.request_buffer.body, &reqlen, &n_request))
    {
        goto err;
    }

    if (es10x_command(ctx, &respbuf, &resplen, ctx->apdu._internal.request_buffer.body, reqlen) < 0)
    {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&tmpnode, n_request.tag, respbuf, resplen) < 0)
    {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&tmpnode, 0x80, tmpnode.value, tmpnode.length) < 0)
    {
        goto err;
    }

    fret = euicc_derutil_convert_bin2long(tmpnode.value, tmpnode.length);

    goto exit;

err:
    fret = -1;
exit:
    free(respbuf);
    respbuf = NULL;
    return fret;
}

void es10a_euicc_configured_addresses_free(struct es10a_euicc_configured_addresses *address)
{
    if (!address)
    {
        return;
    }
    free(address->defaultDpAddress);
    free(address->rootDsAddress);
    memset(address, 0x00, sizeof(struct es10a_euicc_configured_addresses));
}
