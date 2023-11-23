#pragma once
#include <euicc/interface.h>
#include <applet.h>

extern struct euicc_apdu_interface dlsym_apdu_interface;
extern struct euicc_http_interface dlsym_http_interface;
extern struct applet_entry applet_dlsym_interface;

int dlsym_interface_init(void);
