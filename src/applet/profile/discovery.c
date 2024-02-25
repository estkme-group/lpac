#include "discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <main.h>

#include <euicc/es10b.h>
#include <euicc/es9p.h>

static int applet_main(int argc, char **argv)
{
    // int opt;
    // static const char *opt_string = "s:i:h?";

    // char *smds = NULL;
    // char *imei = NULL;

    // char *b64_euicc_challenge = NULL;
    // char *b64_euicc_info_1 = NULL;

    // struct es10b_authenticate_server_param es10b_AuthenticateServer_param;
    // char *b64_authenticate_server_response = NULL;

    // struct es11_authenticate_client_resp es11_AuthenticateClient_resp;

    // opt = getopt(argc, argv, opt_string);
    // while (opt != -1)
    // {
    //     switch (opt)
    //     {
    //     case 's':
    //         smds = strdup(optarg);
    //         break;
    //     case 'i':
    //         imei = strdup(optarg);
    //         break;
    //     case 'h':
    //     case '?':
    //         printf("Usage: %s [OPTIONS]\r\n", argv[0]);
    //         printf("\t -s SM-DS Domain\r\n");
    //         printf("\t -i IMEI\r\n");
    //         printf("\t -h This help info\r\n");
    //         return -1;
    //         break;
    //     }
    //     opt = getopt(argc, argv, opt_string);
    // }

    // if (smds == NULL)
    // {
    //     // smds = "prod.smds.rsp.goog";
    //     // smds = "lpa.live.esimdiscovery.com";
    //     smds = "lpa.ds.gsma.com";
    // }

    // euicc_ctx.http.server_address = smds;

    // jprint_progress("es10b_get_euicc_challenge");
    // if (es10b_get_euicc_challenge_r(&euicc_ctx, &b64_euicc_challenge))
    // {
    //     jprint_error("es10b_get_euicc_challenge", NULL);
    //     return -1;
    // }

    // jprint_progress("es10b_get_euicc_info");
    // if (es10b_get_euicc_info_r(&euicc_ctx, &b64_euicc_info_1))
    // {
    //     jprint_error("es10b_get_euicc_info", NULL);
    //     return -1;
    // }

    // jprint_progress("es9p_InitiateAuthentication");
    // if (es9p_initiate_authentication_r(&euicc_ctx, &es10b_AuthenticateServer_param, b64_euicc_challenge, b64_euicc_info_1))
    // {
    //     jprint_error("es11_initiate_authentication", euicc_ctx.http.status.message);
    //     return -1;
    // }

    // es10b_AuthenticateServer_param.matchingId = NULL;
    // es10b_AuthenticateServer_param.imei = imei;

    // jprint_progress("es10b_authenticate_server");
    // if (es10b_authenticate_server_r(&euicc_ctx, &b64_authenticate_server_response, &es10b_AuthenticateServer_param))
    // {
    //     jprint_error("es10b_authenticate_server", NULL);
    //     return -1;
    // }

    // jprint_progress("es11_authenticate_client");
    // if (es11_authenticate_client_r(&euicc_ctx, &es11_AuthenticateClient_resp, b64_authenticate_server_response))
    // {
    //     jprint_error("es11_authenticate_client", es11_AuthenticateClient_resp.status);
    //     return -1;
    // }

    // jprint_success((cJSON *)es11_AuthenticateClient_resp.cjson_array_result);
    return -1;
}

struct applet_entry applet_profile_discovery = {
    .name = "discovery",
    .main = applet_main,
};
