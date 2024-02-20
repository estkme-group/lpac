#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <main.h>

#include <euicc/es10a.h>
#include <euicc/es10b.h>
#include <euicc/es9p.h>

static int applet_main(int argc, char **argv)
{
    int opt;
    static const char *opt_string = "s:m:i:c:h?";

    char *smdp = NULL;
    char *matchingId = NULL;
    char *imei = NULL;
    char *confirmation_code = NULL;

    struct es10a_euicc_configured_addresses configured_addresses;

    struct es9p_ctx es9p_ctx = {0};

    char *b64_euicc_challenge = NULL;
    char *b64_euicc_info_1 = NULL;

    struct es10b_authenticate_server_param es10b_AuthenticateServer_param = {0};
    char *b64_authenticate_server_response = NULL;

    struct es10b_prepare_download_param es10b_PrepareDownload_param = {0};
    char *b64_prepare_download_response = NULL;

    char *b64_BoundProfilePackage = NULL;

    opt = getopt(argc, argv, opt_string);
    while (opt != -1)
    {
        switch (opt)
        {
        case 's':
            smdp = strdup(optarg);
            break;
        case 'm':
            matchingId = strdup(optarg);
            break;
        case 'i':
            imei = strdup(optarg);
            break;
        case 'c':
            confirmation_code = strdup(optarg);
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS]\r\n", argv[0]);
            printf("\t -s SM-DP+ Domain\r\n");
            printf("\t -m Matching ID\r\n");
            printf("\t -i IMEI\r\n");
            printf("\t -c Confirmation Code (Password)\r\n");
            printf("\t -h This help info\r\n");
            return -1;
            break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    if (smdp == NULL)
    {
        jprint_progress("es10a_GetEuiccConfiguredAddresses");
        if (es10a_GetEuiccConfiguredAddresses(&euicc_ctx, &configured_addresses))
        {
            jprint_error("es10a_GetEuiccConfiguredAddresses", NULL);
            return -1;
        }
        else
        {
            smdp = configured_addresses.defaultDpAddress;
        }
    }

    if (!smdp || (strlen(smdp) == 0))
    {
        jprint_error("smdp is null", NULL);
        return -1;
    }

    es9p_ctx.euicc_ctx = &euicc_ctx;
    es9p_ctx.address = smdp;

    jprint_progress("es10b_GetEUICCChallenge");
    if (es10b_GetEUICCChallenge(&euicc_ctx, &b64_euicc_challenge))
    {
        jprint_error("es10b_GetEUICCChallenge", NULL);
        return -1;
    }

    jprint_progress("es10b_GetEUICCInfo");
    if (es10b_GetEUICCInfo(&euicc_ctx, &b64_euicc_info_1))
    {
        jprint_error("es10b_GetEUICCInfo", NULL);
        return -1;
    }

    jprint_progress("es9p_InitiateAuthentication");
    if (es9p_InitiateAuthentication(&es9p_ctx, &es10b_AuthenticateServer_param, b64_euicc_challenge, b64_euicc_info_1))
    {
        jprint_error("es9p_InitiateAuthentication", es9p_ctx.statusCodeData.message);
        return -1;
    }

    es10b_AuthenticateServer_param.matchingId = matchingId;
    es10b_AuthenticateServer_param.imei = imei;

    jprint_progress("es10b_AuthenticateServer");
    if (es10b_AuthenticateServer(&euicc_ctx, &b64_authenticate_server_response, &es10b_AuthenticateServer_param))
    {
        jprint_error("es10b_AuthenticateServer", NULL);
        return -1;
    }

    jprint_progress("es9p_AuthenticateClient");
    if (es9p_AuthenticateClient(&es9p_ctx, &es10b_PrepareDownload_param, b64_authenticate_server_response))
    {
        jprint_error("es9p_AuthenticateClient", es9p_ctx.statusCodeData.message);
        return -1;
    }

    if (confirmation_code)
    {
        es10b_PrepareDownload_param.confirmationCode = confirmation_code;
    }
    else
    {
        es10b_PrepareDownload_param.confirmationCode = NULL;
    }

    jprint_progress("es10b_PrepareDownload");
    if (es10b_PrepareDownload(&euicc_ctx, &b64_prepare_download_response, &es10b_PrepareDownload_param))
    {
        jprint_error("es10b_PrepareDownload", NULL);
        return -1;
    }

    jprint_progress("es9p_GetBoundProfilePackage");
    if (es9p_GetBoundProfilePackage(&es9p_ctx, &b64_BoundProfilePackage, b64_prepare_download_response))
    {
        jprint_error("es9p_GetBoundProfilePackage", es9p_ctx.statusCodeData.message);
        return -1;
    }

    jprint_progress("es10b_LoadBoundProfilePackage");
    if (es10b_LoadBoundProfilePackage(&euicc_ctx, b64_BoundProfilePackage))
    {
        jprint_error("es10b_LoadBoundProfilePackage", NULL);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_profile_download = {
    .name = "download",
    .main = applet_main,
};
