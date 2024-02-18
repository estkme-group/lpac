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
    struct
    {
        uint8_t apdu_header[5];
        uint8_t body[255];
    } __attribute__((packed)) apdu_request_buffer;
    int es10x_logic_channel;
    void *userdata;
};

#include "es9p.h"
#include "es10x.h"
