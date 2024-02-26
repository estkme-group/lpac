#pragma once

#include <inttypes.h>
#include "interface.h"
#include "es10b.h"

struct euicc_ctx
{
    struct
    {
        const struct euicc_apdu_interface *interface;
        struct
        {
            int logic_channel;
            struct
            {
                uint8_t apdu_header[5];
                uint8_t body[255];
            } __attribute__((packed)) request_buffer;
        } _internal;
    } apdu;
    struct
    {
        const struct euicc_http_interface *interface;
        const char *server_address;
        struct
        {
            char subjectCode[8 + 1];
            char reasonCode[8 + 1];
            char subjectIdentifier[128 + 1];
            char message[128 + 1];
        } status;
        struct
        {
            char *transaction_id;
            char *b64_euicc_challenge;
            char *b64_euicc_info_1;
            struct es10b_authenticate_server_param *authenticate_server_param;
            char *b64_authenticate_server_response;
            struct es10b_prepare_download_param *prepare_download_param;
            char *b64_prepare_download_response;
            char *b64_bound_profile_package;
        } _internal;
    } http;
    void *userdata;
};

int euicc_init(struct euicc_ctx *ctx);
void euicc_fini(struct euicc_ctx *ctx);
void euicc_http_cleanup(struct euicc_ctx *ctx);
