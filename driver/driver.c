#include "driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LPAC_WITH_APDU_GBINDER
#include "driver/apdu/gbinder_hidl.h"
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

static const struct lpac_driver *drivers[] = {
#ifdef LPAC_WITH_APDU_GBINDER
    &driver_apdu_gbinder_hidl,
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

static const struct lpac_driver *_driver_apdu = NULL;
static const struct lpac_driver *_driver_http = NULL;

struct euicc_apdu_interface driver_interface_apdu;
struct euicc_http_interface driver_interface_http;
int (*driver_main_apdu)(int argc, char **argv) = NULL;
int (*driver_main_http)(int argc, char **argv) = NULL;

static const struct lpac_driver *_find_driver(enum lpac_driver_type type, const char *name)
{
    for (int i = 0; drivers[i] != NULL; i++)
    {
        const struct lpac_driver *d = drivers[i];
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

int driver_init()
{
    _driver_apdu = _find_driver(DRIVER_APDU, getenv("LPAC_APDU"));
    if (_driver_apdu == NULL)
    {
        fprintf(stderr, "No APDU driver found\n");
        return -1;
    }

    _driver_http = _find_driver(DRIVER_HTTP, getenv("LPAC_HTTP"));
    if (_driver_http == NULL)
    {
        fprintf(stderr, "No HTTP driver found\n");
        return -1;
    }

    _driver_apdu->init(&driver_interface_apdu);
    _driver_http->init(&driver_interface_http);

    driver_main_apdu = _driver_apdu->main;
    driver_main_http = _driver_http->main;

    return 0;
}

void driver_fini()
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
