#pragma once

#include <inttypes.h>

#include "interface.h"

struct euicc_ctx
{
    struct
    {
        struct euicc_apdu_interface *apdu;
        struct euicc_http_interface *http;
    } interface;
    uint8_t g_apdu_request_buf[256 + 8];
    uint8_t g_asn1_der_request_buf[256];
    int es10x_logic_channel;
    void *userdata;
};

#include "es9p.h"
#include "es10x.h"
