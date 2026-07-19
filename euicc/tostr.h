#pragma once
#include "euicc_export.h"

#include "es10b.h"
#include "es10c.h"

#include <inttypes.h>
#include <stddef.h>

EUICC_API const char *euicc_profilestate2str(enum es10c_profile_state value);
EUICC_API const char *euicc_profileclass2str(enum es10c_profile_class value);
EUICC_API const char *euicc_icontype2str(enum es10c_icon_type value);
EUICC_API const char *euicc_profilemanagementoperation2str(enum es10b_profile_management_operation value);
EUICC_API const char *euicc_bppcommandid2str(enum es10b_bpp_command_id value);
EUICC_API const char *euicc_errorreason2str(enum es10b_error_reason value);
