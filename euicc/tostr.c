#include "tostr.h"

const char *profilestate2str(enum es10c_profile_state value)
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
}

const char *profileclass2str(enum es10c_profile_class value)
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
}

const char *icontype2str(enum es10c_icon_type value)
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
}

const char *profilemanagementoperation2str(enum es10b_profile_management_operation value)
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
}
