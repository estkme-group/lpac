#pragma once

#include <stdbool.h>

#include <cjson/cJSON.h>
#include <euicc/es10b.h>

char *notification_strstrip(char *input);

bool build_notification(cJSON **jroot, const char *eid, uint32_t seqNumber,
                       const struct es10b_pending_notification *notification);

bool parse_notification(const cJSON *jroot, const char *eid, uint32_t *seqNumber,
                       struct es10b_pending_notification *notification);
