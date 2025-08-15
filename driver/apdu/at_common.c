#include "at_common.h"

#include "cjson/cJSON_ex.h"

bool jprint_enumerate_devices(cJSON *data) {
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringOrNullToObject(payload, "env", ENV_AT_DEVICE);
    cJSON_AddItemToObject(payload, "data", data);
    return json_print("driver", payload);
}
