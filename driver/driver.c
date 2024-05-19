#include "driver.h"
#include "driver.private.h"

#include <stdio.h>
#include <string.h>

#ifdef LPAC_WITH_APDU_GBINDER
#include "driver/apdu/gbinder_hidl.h"
#endif

#ifdef LPAC_WITH_APDU_QMI_QRTR
#include "driver/apdu/qmi_qrtr.h"
#endif

#ifdef LPAC_WITH_APDU_PCSC
#include "driver/apdu/pcsc.h"
#endif
#ifdef LPAC_WITH_APDU_AT
#include "driver/apdu/at.h"
#endif
#ifdef LPAC_WITH_HTTP_CURL
#include "driver/http/curl.h"
#endif
#include "driver/apdu/stdio.h"
#include "driver/http/stdio.h"

static const struct euicc_driver *drivers[] = {
#ifdef LPAC_WITH_APDU_GBINDER
    &driver_apdu_gbinder_hidl,
#endif
#ifdef LPAC_WITH_APDU_QMI_QRTR
    &driver_apdu_qmi_qrtr,
#endif
#ifdef LPAC_WITH_APDU_PCSC
    &driver_apdu_pcsc,
#endif
#ifdef LPAC_WITH_APDU_AT
    &driver_apdu_at,
#endif
#ifdef LPAC_WITH_HTTP_CURL
    &driver_http_curl,
#endif
    &driver_apdu_stdio,
    &driver_http_stdio,
    NULL,
};

static const struct euicc_driver *_driver_apdu = NULL;
static const struct euicc_driver *_driver_http = NULL;

struct euicc_apdu_interface euicc_driver_interface_apdu;
struct euicc_http_interface euicc_driver_interface_http;
int (*euicc_driver_main_apdu)(int argc, char **argv) = NULL;
int (*euicc_driver_main_http)(int argc, char **argv) = NULL;

static const struct euicc_driver *_find_driver(enum euicc_driver_type type, const char *name)
{
    for (int i = 0; drivers[i] != NULL; i++)
    {
        const struct euicc_driver *d = drivers[i];
        if (d->type != type)
        {
            continue;
        }
        if (name == NULL)
        {
            return d;
        }
        if (strcmp(d->name, name) == 0)
        {
            return d;
        }
    }
    return NULL;
}

int euicc_driver_init(const char *apdu_driver_name, const char *http_driver_name)
{
    _driver_apdu = _find_driver(DRIVER_APDU, apdu_driver_name);
    if (_driver_apdu == NULL)
    {
        fprintf(stderr, "No APDU driver found\n");
        return -1;
    }

    _driver_http = _find_driver(DRIVER_HTTP, http_driver_name);
    if (_driver_http == NULL)
    {
        fprintf(stderr, "No HTTP driver found\n");
        return -1;
    }

    if (_driver_apdu->init(&euicc_driver_interface_apdu))
    {
        fprintf(stderr, "APDU driver init failed\n");
        return -1;
    }

    if (_driver_http->init(&euicc_driver_interface_http))
    {
        fprintf(stderr, "HTTP driver init failed\n");
        return -1;
    }

    euicc_driver_main_apdu = _driver_apdu->main;
    euicc_driver_main_http = _driver_http->main;

    return 0;
}

void euicc_driver_fini()
{
    if (_driver_apdu != NULL)
    {
        _driver_apdu->fini();
    }
    if (_driver_http != NULL)
    {
        _driver_http->fini();
    }
}
