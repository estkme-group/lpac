#include "driver.h"
#include "driver.private.h"

#include <lpac/utils.h>

#include <stdio.h>
#include <string.h>

#ifdef LPAC_WITH_APDU_GBINDER
#    include "driver/apdu/gbinder_hidl.h"
#endif

#ifdef LPAC_WITH_APDU_MBIM
#    include "driver/apdu/mbim.h"
#endif

#ifdef LPAC_WITH_APDU_QMI
#    include "driver/apdu/qmi.h"
#endif

#ifdef LPAC_WITH_APDU_QMI_QRTR
#    include "driver/apdu/qmi_qrtr.h"
#endif

#ifdef LPAC_WITH_APDU_PCSC
#    include "driver/apdu/pcsc.h"
#endif
#ifdef LPAC_WITH_APDU_AT
#    include "driver/apdu/at.h"
#endif
#ifdef LPAC_WITH_HTTP_CURL
#    include "driver/http/curl.h"
#endif
#include "driver/apdu/stdio.h"
#include "driver/http/stdio.h"

static const struct euicc_driver *drivers[] = {
#ifdef LPAC_WITH_APDU_GBINDER
    &driver_apdu_gbinder_hidl,
#endif
#ifdef LPAC_WITH_APDU_MBIM
    &driver_apdu_mbim,
#endif
#ifdef LPAC_WITH_APDU_QMI
    &driver_apdu_qmi,
#endif
#ifdef LPAC_WITH_APDU_QMI_QRTR
    &driver_apdu_qmi_qrtr,
#endif
#ifdef LPAC_WITH_APDU_PCSC
    &driver_apdu_pcsc,
#endif
#ifdef LPAC_WITH_APDU_AT
    &driver_apdu_at,           &driver_apdu_at_csim,
#endif
#ifdef LPAC_WITH_HTTP_CURL
    &driver_http_curl,
#endif
    &driver_apdu_stdio,        &driver_http_stdio,   NULL,
};

static const struct euicc_driver *_driver_apdu = NULL;
static const struct euicc_driver *_driver_http = NULL;

struct euicc_apdu_interface euicc_driver_interface_apdu;
struct euicc_http_interface euicc_driver_interface_http;

static const struct euicc_driver *find_driver(const enum euicc_driver_type type, const char *name) {
    for (int i = 0; drivers[i] != NULL; i++) {
        const struct euicc_driver *d = drivers[i];
        if (d->type != type) {
            continue;
        }
        if (name == NULL) {
            return d;
        }
        if (strcmp(d->name, name) == 0) {
            return d;
        }
    }
    return NULL;
}

int euicc_driver_list(int argc, char **argv) {
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
        return -1;

    const struct euicc_driver *driver = NULL;
    cJSON *driver_name = NULL;
    cJSON *apdu_drivers = cJSON_CreateArray();
    cJSON *http_drivers = cJSON_CreateArray();
    if (apdu_drivers == NULL)
        return -1;
    for (int i = 0; drivers[i] != NULL; i++) {
        driver = drivers[i];
        driver_name = cJSON_CreateString(driver->name);
        if (driver_name == NULL)
            return -1;
        if (driver->type == DRIVER_APDU) {
            cJSON_AddItemToArray(apdu_drivers, driver_name);
        } else if (driver->type == DRIVER_HTTP) {
            cJSON_AddItemToArray(http_drivers, driver_name);
        }
    }
    cJSON_AddItemToObject(payload, ENV_APDU_DRIVER, apdu_drivers);
    cJSON_AddItemToObject(payload, ENV_HTTP_DRIVER, http_drivers);

    json_print("driver", payload);
    return 0;
}

int euicc_driver_init(const char *apdu_driver_name, const char *http_driver_name) {
    _driver_apdu = find_driver(DRIVER_APDU, apdu_driver_name);
    if (_driver_apdu == NULL) {
        fprintf(stderr, "No APDU driver found\n");
        return -1;
    }

    _driver_http = find_driver(DRIVER_HTTP, http_driver_name);
    if (_driver_http == NULL) {
        fprintf(stderr, "No HTTP driver found\n");
        return -1;
    }

    if (_driver_apdu->init(&euicc_driver_interface_apdu)) {
        fprintf(stderr, "APDU driver init failed\n");
        return -1;
    }

    if (_driver_http->init(&euicc_driver_interface_http)) {
        fprintf(stderr, "HTTP driver init failed\n");
        return -1;
    }

    return 0;
}

void euicc_driver_fini() {
    if (_driver_apdu != NULL) {
        _driver_apdu->fini(&euicc_driver_interface_apdu);
    }
    if (_driver_http != NULL) {
        _driver_http->fini(&euicc_driver_interface_http);
    }
}

int euicc_driver_main_apdu(const int argc, char **argv) {
    if (_driver_apdu == NULL) {
        fprintf(stderr, "No APDU driver found\n");
        return -1;
    }
    if (_driver_apdu->main == NULL) {
        fprintf(stderr, "The APDU driver '%s' does not support main function\n", _driver_apdu->name);
        return -1;
    }
    return _driver_apdu->main(&euicc_driver_interface_apdu, argc, argv);
}

int euicc_driver_main_http(const int argc, char **argv) {
    if (_driver_http == NULL) {
        fprintf(stderr, "No HTTP driver found\n");
        return -1;
    }
    if (_driver_http->main == NULL) {
        fprintf(stderr, "The HTTP driver '%s' does not support main function\n", _driver_http->name);
        return -1;
    }
    return _driver_http->main(&euicc_driver_interface_http, argc, argv);
}
