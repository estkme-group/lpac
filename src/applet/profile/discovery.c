#include "discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <main.h>

static int applet_main(int argc, char **argv)
{
    int opt;
    static const char *opt_string = "s:i:h?";

    char *transaction_id = NULL;

    char *smds = NULL;
    char *imei = NULL;

    char *b64_euicc_challenge = NULL;
    char *b64_euicc_info_1 = NULL;
    struct es9p_initiate_authentication_resp es11_initiate_authentication_resp;

    struct es10b_authenticate_server_param es10b_authenticate_server_param;
    char *b64_authenticate_server_response = NULL;

    struct es11_authenticate_client_resp es11_authenticate_client_resp;

    opt = getopt(argc, argv, opt_string);
    while (opt != -1)
    {
        switch (opt)
        {
        case 's':
            smds = strdup(optarg);
            break;
        case 'i':
            imei = strdup(optarg);
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS]\r\n", argv[0]);
            printf("\t -s SM-DS Domain\r\n");
            printf("\t -i IMEI\r\n");
            printf("\t -h This help info\r\n");
            return -1;
            break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    if (smds == NULL)
    {
        // smds = "prod.smds.rsp.goog";
        // smds = "lpa.live.esimdiscovery.com";
        smds = "lpa.ds.gsma.com";
    }

    jprint_progress("es10b_get_euicc_challenge");
    if (es10b_get_euicc_challenge(&euicc_ctx, &b64_euicc_challenge))
    {
        jprint_error("es10b_get_euicc_challenge", NULL);
        return -1;
    }

    jprint_progress("es10b_get_euicc_info");
    if (es10b_get_euicc_info(&euicc_ctx, &b64_euicc_info_1))
    {
        jprint_error("es10b_get_euicc_info", NULL);
        return -1;
    }

    jprint_progress("es9p_initiate_authentication");
    if (es9p_initiate_authentication(&euicc_ctx, smds, b64_euicc_challenge, b64_euicc_info_1, &es11_initiate_authentication_resp))
    {
        jprint_error("es11_initiate_authentication", es11_initiate_authentication_resp.status);
        return -1;
    }

    transaction_id = strdup(es11_initiate_authentication_resp.transaction_id);

    es10b_authenticate_server_param.b64_server_signed_1 = es11_initiate_authentication_resp.b64_server_signed_1;
    es10b_authenticate_server_param.b64_server_signature_1 = es11_initiate_authentication_resp.b64_server_signature_1;
    es10b_authenticate_server_param.b64_euicc_ci_pkid_to_be_used = es11_initiate_authentication_resp.b64_euicc_ci_pkid_to_be_used;
    es10b_authenticate_server_param.b64_server_certificate = es11_initiate_authentication_resp.b64_server_certificate;
    es10b_authenticate_server_param.matchingId = NULL;
    es10b_authenticate_server_param.imei = imei;
    es10b_authenticate_server_param.tac = NULL;

    jprint_progress("es10b_authenticate_server");
    if (es10b_authenticate_server(&euicc_ctx, &b64_authenticate_server_response, &es10b_authenticate_server_param))
    {
        jprint_error("es10b_authenticate_server", NULL);
        return -1;
    }

    jprint_progress("es11_authenticate_client");
    if (es11_authenticate_client(&euicc_ctx, smds, transaction_id, b64_authenticate_server_response, &es11_authenticate_client_resp))
    {
        jprint_error("es11_authenticate_client", es11_authenticate_client_resp.status);
        return -1;
    }

    jprint_success((cJSON *)es11_authenticate_client_resp.cjson_array_result);
    return 0;
}

struct applet_entry applet_profile_discovery = {
    .name = "discovery",
    .main = applet_main,
};
