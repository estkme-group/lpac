#pragma once
#include <inttypes.h>
#include <stddef.h>

#include "es10c.h"
#include "es10b.h"

const char *profilestate2str(enum es10c_profile_state value);
const char *profileclass2str(enum es10c_profile_class value);
const char *icontype2str(enum es10c_icon_type value);
const char *profilemanagementoperation2str(enum es10b_profile_management_operation value);
const char *bppcommandid2str(enum es10b_bpp_command_id value);
const char *errorreason2str(enum es10b_error_reason value);
