#pragma once

#include "es10b.h"
#include <stdbool.h>

int es10b_parse_install_pending_notification(const struct es10b_pending_notification *,
                                             struct es10b_load_bound_profile_package_result *);

int es10b_parse_profile_installation_result(const uint8_t *buf, unsigned int buf_len,
                                            struct es10b_load_bound_profile_package_result *result);
