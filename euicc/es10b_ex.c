#include "es10b_ex.h"

#include "base64.h"
#include "derutil.h"

#include <stdlib.h>

int es10b_parse_install_pending_notification(const struct es10b_pending_notification *notification,
                                             struct es10b_load_bound_profile_package_result *result) {
    int fret = 0;
    const int buf_len = euicc_base64_decode_len(notification->b64_PendingNotification);
    uint8_t *buf = calloc(buf_len, sizeof(uint8_t));
    if (euicc_base64_decode(buf, notification->b64_PendingNotification) < 0) {
        goto err;
    }
    if (es10b_parse_profile_installation_result(buf, buf_len, result) < 0) {
        goto err;
    }
    goto exit;
err:
    fret = -1;
exit:
    free(buf);
    buf = NULL;
    return fret;
}

int es10b_parse_profile_installation_result(const uint8_t *buf, const unsigned int buf_len,
                                            struct es10b_load_bound_profile_package_result *result) {
    struct euicc_derutil_node tmpnode, n_notificationMetadata, n_sequenceNumber, n_finalResult;

    if (euicc_derutil_unpack_find_tag(&tmpnode, 0xBF37, buf, buf_len) < 0) {
        goto err; // ProfileInstallationResult
    }

    if (euicc_derutil_unpack_find_tag(&tmpnode, 0xBF27, tmpnode.value, tmpnode.length) < 0) {
        goto err; // ProfileInstallationResultData
    }

    if (euicc_derutil_unpack_find_tag(&n_notificationMetadata, 0xBF2F, tmpnode.value, tmpnode.length) < 0) {
        goto err; // NotificationMetadata
    }

    if (euicc_derutil_unpack_find_tag(&tmpnode, 0xA2, tmpnode.value, tmpnode.length) < 0) {
        goto err; // finalResult
    }

    if (euicc_derutil_unpack_first(&n_finalResult, tmpnode.value, tmpnode.length) < 0) {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&n_sequenceNumber, 0x80, n_notificationMetadata.value,
                                      n_notificationMetadata.length)
        == 0) {
        result->seqNumber = euicc_derutil_convert_bin2long(n_sequenceNumber.value, n_sequenceNumber.length);
    }

    switch (n_finalResult.tag) {
    case 0xA0: // SuccessResult
        break;
    case 0xA1: // ErrorResult
        tmpnode.self.ptr = n_finalResult.value;
        tmpnode.self.length = 0;
        while (euicc_derutil_unpack_next(&tmpnode, &tmpnode, n_finalResult.value, n_finalResult.length) == 0) {
            long tmpint;
            switch (tmpnode.tag) {
            case 0x80:
                tmpint = euicc_derutil_convert_bin2long(tmpnode.value, tmpnode.length);
                switch (tmpint) {
                case ES10B_BPP_COMMAND_ID_INITIALISE_SECURE_CHANNEL:
                case ES10B_BPP_COMMAND_ID_CONFIGURE_ISDP:
                case ES10B_BPP_COMMAND_ID_STORE_METADATA:
                case ES10B_BPP_COMMAND_ID_STORE_METADATA2:
                case ES10B_BPP_COMMAND_ID_REPLACE_SESSION_KEYS:
                case ES10B_BPP_COMMAND_ID_LOAD_PROFILE_ELEMENTS:
                    result->bppCommandId = tmpint;
                    break;
                default:
                    result->bppCommandId = ES10B_BPP_COMMAND_ID_UNDEFINED;
                    break;
                }
                break;
            case 0x81:
                tmpint = euicc_derutil_convert_bin2long(tmpnode.value, tmpnode.length);
                switch (tmpint) {
                case ES10B_ERROR_REASON_INCORRECT_INPUT_VALUES:
                case ES10B_ERROR_REASON_INVALID_SIGNATURE:
                case ES10B_ERROR_REASON_INVALID_TRANSACTION_ID:
                case ES10B_ERROR_REASON_UNSUPPORTED_CRT_VALUES:
                case ES10B_ERROR_REASON_UNSUPPORTED_REMOTE_OPERATION_TYPE:
                case ES10B_ERROR_REASON_UNSUPPORTED_PROFILE_CLASS:
                case ES10B_ERROR_REASON_SCP03T_STRUCTURE_ERROR:
                case ES10B_ERROR_REASON_SCP03T_SECURITY_ERROR:
                case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_ICCID_ALREADY_EXISTS_ON_EUICC:
                case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_INSUFFICIENT_MEMORY_FOR_PROFILE:
                case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_INTERRUPTION:
                case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_PE_PROCESSING_ERROR:
                case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_DATA_MISMATCH:
                case ES10B_ERROR_REASON_TEST_PROFILE_INSTALL_FAILED_DUE_TO_INVALID_NAA_KEY:
                case ES10B_ERROR_REASON_PPR_NOT_ALLOWED:
                case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_UNKNOWN_ERROR:
                    result->errorReason = tmpint;
                    break;
                default:
                    result->errorReason = ES10B_ERROR_REASON_UNDEFINED;
                    break;
                }
                break;
            default:
                break;
            }
        }
        goto err;
    default: // Unexpected tag
        goto err;
    }
    return 0;
err:
    result->seqNumber = -1;
    result->bppCommandId = ES10B_BPP_COMMAND_ID_UNDEFINED;
    result->errorReason = ES10B_ERROR_REASON_UNDEFINED;
    return -1;
}
