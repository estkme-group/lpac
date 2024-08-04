#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "es9p_ex.h"
#include "hexutil.h"
#include "derutil.h"
#include "base64.h"

int es9p_ex_get_profile_metadata(struct euicc_ctx *ctx, struct es9p_ex_profile_metadata *profile_metadata)
{
    int fret = 0;

    struct euicc_derutil_node n_request = {
        .tag = 0xBF25, // StoreMetadataRequest
    };

    struct euicc_derutil_node tmpnode, n_ProfileMetadata;

    uint8_t *profile_metadata_buffer = malloc(euicc_base64_decode_len(ctx->http._internal.prepare_download_param->b64_profileMetadata));
    if (!profile_metadata_buffer)
    {
        goto err;
    }

    uint32_t profile_metadata_buffer_len = euicc_base64_decode(profile_metadata_buffer, ctx->http._internal.prepare_download_param->b64_profileMetadata);
    if (profile_metadata_buffer_len < 0)
    {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&n_ProfileMetadata, n_request.tag, profile_metadata_buffer, profile_metadata_buffer_len) < 0)
    {
        goto err;
    }

    profile_metadata->profileClass = ES9P_EX_PROFILE_CLASS_NULL;
    profile_metadata->iconType = ES9P_EX_ICON_TYPE_NULL;
    profile_metadata->icon = NULL;

    tmpnode.self.ptr = n_ProfileMetadata.value;
    tmpnode.self.length = 0;
    while (euicc_derutil_unpack_next(&tmpnode, &tmpnode, n_ProfileMetadata.value, n_ProfileMetadata.length) == 0)
    {
        long tmpint;
        switch (tmpnode.tag)
        {
        case 0x5A: // ICCID
            euicc_hexutil_bin2gsmbcd(profile_metadata->iccid, sizeof(profile_metadata->iccid), tmpnode.value, tmpnode.length);
            break;
        case 0X91: // ServiceProviderName
            profile_metadata->serviceProviderName = malloc(tmpnode.length + 1);
            if (profile_metadata->serviceProviderName)
            {
                memcpy(profile_metadata->serviceProviderName, tmpnode.value, tmpnode.length);
                profile_metadata->serviceProviderName[tmpnode.length] = '\0';
            }
            break;
        case 0x92: // Profile Name
            profile_metadata->profileName = malloc(tmpnode.length + 1);
            if (profile_metadata->profileName)
            {
                memcpy(profile_metadata->profileName, tmpnode.value, tmpnode.length);
                profile_metadata->profileName[tmpnode.length] = '\0';
            }
            break;
        case 0x93: // Icon Type
            tmpint = euicc_derutil_convert_bin2long(tmpnode.value, tmpnode.length);
            switch (tmpint)
            {
            case ES9P_EX_ICON_TYPE_JPEG:
            case ES9P_EX_ICON_TYPE_PNG:
                profile_metadata->iconType = tmpint;
                break;
            default:
                profile_metadata->iconType = ES9P_EX_ICON_TYPE_UNDEFINED;
                break;
            }
            break;
        case 0x94: // Icon
            profile_metadata->icon = malloc(euicc_base64_encode_len(tmpnode.length));
            if (profile_metadata->icon)
            {
                euicc_base64_encode(profile_metadata->icon, tmpnode.value, tmpnode.length);
            }
            break;
        case 0x95: // Profile Class
            tmpint = euicc_derutil_convert_bin2long(tmpnode.value, tmpnode.length);
            switch (tmpint)
            {
            case ES9P_EX_PROFILE_CLASS_TEST:
            case ES9P_EX_PROFILE_CLASS_PROVISIONING:
            case ES9P_EX_PROFILE_CLASS_OPERATIONAL:
                profile_metadata->profileClass = tmpint;
                break;
            default:
                profile_metadata->profileClass = ES9P_EX_PROFILE_CLASS_UNDEFINED;
                break;
            }
            break;
        default:
            break;
        }
    }
    
    free(profile_metadata_buffer);
    goto exit;
err:
    fret = -1;
exit:
    return fret;
}

void es9p_ex_free(struct es9p_ex_profile_metadata *profile_metadata)
{
    if (profile_metadata->serviceProviderName)
    {
        free(profile_metadata->serviceProviderName);
    }
    if (profile_metadata->profileName)
    {
        free(profile_metadata->profileName);
    }
    if (profile_metadata->icon)
    {
        free(profile_metadata->icon);
    }
}
