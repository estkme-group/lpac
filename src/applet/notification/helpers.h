#pragma once

#include <cjson/cJSON.h>
#include <euicc/es10b.h>

char *notification_strstrip(char *input);

int build_notification(cJSON **jroot, const char *eid, uint32_t seqNumber,
                       const struct es10b_pending_notification *notification);

int parse_notification(const cJSON *jroot, const char *eid, uint32_t *seqNumber,
                       struct es10b_pending_notification *notification);
