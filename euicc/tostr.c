#include "tostr.h"

const char *euicc_profilestate2str(enum es10c_profile_state value)
{
    switch (value)
    {
    case ES10C_PROFILE_STATE_NULL:
        return NULL;
    case ES10C_PROFILE_STATE_DISABLED:
        return "disabled";
    case ES10C_PROFILE_STATE_ENABLED:
        return "enabled";
    case ES10C_PROFILE_STATE_UNDEFINED:
        return "unknown";
    }
    return "(no_str_available)";
}

const char *euicc_profileclass2str(enum es10c_profile_class value)
{
    switch (value)
    {
    case ES10C_PROFILE_CLASS_NULL:
        return NULL;
    case ES10C_PROFILE_CLASS_TEST:
        return "test";
    case ES10C_PROFILE_CLASS_PROVISIONING:
        return "provisioning";
    case ES10C_PROFILE_CLASS_OPERATIONAL:
        return "operational";
    case ES10C_PROFILE_CLASS_UNDEFINED:
        return "unknown";
    }
    return "(no_str_available)";
}

const char *euicc_icontype2str(enum es10c_icon_type value)
{
    switch (value)
    {
    case ES10C_ICON_TYPE_NULL:
        return NULL;
    case ES10C_ICON_TYPE_JPEG:
        return "jpeg";
    case ES10C_ICON_TYPE_PNG:
        return "png";
    case ES10C_ICON_TYPE_UNDEFINED:
        return "unknown";
    }
    return "(no_str_available)";
}

const char *euicc_profilemanagementoperation2str(enum es10b_profile_management_operation value)
{
    switch (value)
    {
    case ES10B_PROFILE_MANAGEMENT_OPERATION_NULL:
        return NULL;
    case ES10B_PROFILE_MANAGEMENT_OPERATION_INSTALL:
        return "install";
    case ES10B_PROFILE_MANAGEMENT_OPERATION_ENABLE:
        return "enable";
    case ES10B_PROFILE_MANAGEMENT_OPERATION_DISABLE:
        return "disable";
    case ES10B_PROFILE_MANAGEMENT_OPERATION_DELETE:
        return "delete";
    case ES10B_PROFILE_MANAGEMENT_OPERATION_UNDEFINED:
        return "unknown";
    }
    return "(no_str_available)";
}

const char *euicc_bppcommandid2str(enum es10b_bpp_command_id value)
{
    switch (value)
    {
    case ES10B_BPP_COMMAND_ID_INITIALISE_SECURE_CHANNEL:
        return "initialise_secure_channel";
    case ES10B_BPP_COMMAND_ID_CONFIGURE_ISDP:
        return "configure_isdp";
    case ES10B_BPP_COMMAND_ID_STORE_METADATA:
        return "store_metadata";
    case ES10B_BPP_COMMAND_ID_STORE_METADATA2:
        return "store_metadata2";
    case ES10B_BPP_COMMAND_ID_REPLACE_SESSION_KEYS:
        return "replace_session_keys";
    case ES10B_BPP_COMMAND_ID_LOAD_PROFILE_ELEMENTS:
        return "load_profile_elements";
    case ES10B_BPP_COMMAND_ID_UNDEFINED:
        return "unknown";
    }
    return "(no_str_available)";
}

const char *euicc_errorreason2str(enum es10b_error_reason value)
{
    switch (value)
    {
    case ES10B_ERROR_REASON_INCORRECT_INPUT_VALUES:
        return "incorrect_input_values";
    case ES10B_ERROR_REASON_INVALID_SIGNATURE:
        return "invalid_signature";
    case ES10B_ERROR_REASON_INVALID_TRANSACTION_ID:
        return "invalid_transaction_id";
    case ES10B_ERROR_REASON_UNSUPPORTED_CRT_VALUES:
        return "unsupported_crt_values";
    case ES10B_ERROR_REASON_UNSUPPORTED_REMOTE_OPERATION_TYPE:
        return "unsupported_remote_operation_type";
    case ES10B_ERROR_REASON_UNSUPPORTED_PROFILE_CLASS:
        return "unsupported_profile_class";
    case ES10B_ERROR_REASON_SCP03T_STRUCTURE_ERROR:
        return "scp03t_structure_error";
    case ES10B_ERROR_REASON_SCP03T_SECURITY_ERROR:
        return "scp03t_security_error";
    case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_ICCID_ALREADY_EXISTS_ON_EUICC:
        return "install_failed_due_to_iccid_already_exists_on_euicc";
    case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_INSUFFICIENT_MEMORY_FOR_PROFILE:
        return "install_failed_due_to_insufficient_memory_for_profile";
    case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_INTERRUPTION:
        return "install_failed_due_to_interruption";
    case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_PE_PROCESSING_ERROR:
        return "install_failed_due_to_pe_processing_error";
    case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_ICCID_MISMATCH:
        return "install_failed_due_to_iccid_mismatch";
    case ES10B_ERROR_REASON_TEST_PROFILE_INSTALL_FAILED_DUE_TO_INVALID_NAA_KEY:
        return "test_profile_install_failed_due_to_invalid_naa_key";
    case ES10B_ERROR_REASON_PPR_NOT_ALLOWED:
        return "ppr_not_allowed";
    case ES10B_ERROR_REASON_INSTALL_FAILED_DUE_TO_UNKNOWN_ERROR:
        return "install_failed_due_to_unknown_error";
    case ES10B_ERROR_REASON_UNDEFINED:
        return "unknown";
    }
    return "(no_str_available)";
}
