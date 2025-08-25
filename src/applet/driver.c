#include <driver.h>
#include <cjson/cJSON_ex.h>
#include <jprint.h>
#include <stddef.h>
#include <applet.h>


const struct applet_entry applet_apdu = {
    .name = "apdu",
    .main = euicc_driver_main_apdu,
    .skip_init_euicc = true
};

const struct applet_entry applet_http = {
    .name = "http",
    .main = euicc_driver_main_http,
    .skip_init_euicc = true
};

const struct applet_entry applet_list = {
    .name = "list",
    .main = euicc_driver_list,
    .skip_init_driver = true,
    .skip_init_euicc = true
};

const struct applet_entry *applets[] = {
    &applet_apdu,
    &applet_http,
    &applet_list,
    NULL,
};

struct applet_entry applet_driver = {
    .name = "driver",
    .main = NULL,
    .subapplets = applets,
};
