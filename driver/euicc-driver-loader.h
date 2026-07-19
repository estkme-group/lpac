#pragma once

#include "driver.h"
#include <euicc/interface.h>

#include <inttypes.h>
#include <stddef.h>

extern DRIVER_API struct euicc_apdu_interface euicc_driver_interface_apdu;
extern DRIVER_API struct euicc_http_interface euicc_driver_interface_http;

DRIVER_API int euicc_driver_list(int argc, char **argv);
DRIVER_API int euicc_driver_init(const char *apdu_driver_name, const char *http_driver_name);
DRIVER_API void euicc_driver_fini(void);

DRIVER_API int euicc_driver_main_apdu(int argc, char **argv);
DRIVER_API int euicc_driver_main_http(int argc, char **argv);
