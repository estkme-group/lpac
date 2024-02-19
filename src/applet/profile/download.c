#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <main.h>

static int applet_main(int argc, char **argv)
{
    int opt;
    static const char *opt_string = "s:m:i:c:h?";

    char *transaction_id = NULL;

    char *smdp = NULL;
    char *matchingId = NULL;
    char *imei = NULL;
    char *confirmation_code = NULL;

    struct es10a_euicc_configured_addresses configured_addresses;

    char *b64_euicc_challenge = NULL;
    char *b64_euicc_info_1 = NULL;
    struct es9p_initiate_authentication_resp es9p_initiate_authentication_resp;

    struct es10b_AuthenticateServer_param es10b_AuthenticateServer_param;
    char *b64_authenticate_server_response = NULL;

    struct es9p_authenticate_client_resp es9p_authenticate_client_resp;

    struct es10b_PrepareDownload_param es10b_PrepareDownload_param;
    char *b64_prepare_download_response = NULL;

    struct es9p_get_bound_profile_package_resp es9p_get_bound_profile_package_resp;

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

    jprint_progress("es9p_initiate_authentication");
    if (es9p_initiate_authentication(&euicc_ctx, smdp, b64_euicc_challenge, b64_euicc_info_1, &es9p_initiate_authentication_resp))
    {
        jprint_error("es9p_initiate_authentication", es9p_initiate_authentication_resp.status);
        return -1;
    }

    transaction_id = strdup(es9p_initiate_authentication_resp.transaction_id);
    es10b_AuthenticateServer_param.b64_serverSigned1 = es9p_initiate_authentication_resp.b64_server_signed_1;
    es10b_AuthenticateServer_param.b64_serverSignature1 = es9p_initiate_authentication_resp.b64_server_signature_1;
    es10b_AuthenticateServer_param.b64_euiccCiPKIdToBeUsed = es9p_initiate_authentication_resp.b64_euicc_ci_pkid_to_be_used;
    es10b_AuthenticateServer_param.b64_serverCertificate = es9p_initiate_authentication_resp.b64_server_certificate;
    es10b_AuthenticateServer_param.matchingId = matchingId;
    es10b_AuthenticateServer_param.imei = imei;
    es10b_AuthenticateServer_param.tac = NULL;

    jprint_progress("es10b_AuthenticateServer");
    if (es10b_AuthenticateServer(&euicc_ctx, &b64_authenticate_server_response, &es10b_AuthenticateServer_param))
    {
        jprint_error("es10b_AuthenticateServer", NULL);
        return -1;
    }

    jprint_progress("es9p_authenticate_client");
    if (es9p_authenticate_client(&euicc_ctx, smdp, transaction_id, b64_authenticate_server_response, &es9p_authenticate_client_resp))
    {
        jprint_error("es9p_authenticate_client", es9p_authenticate_client_resp.status);
        return -1;
    }

    es10b_PrepareDownload_param.b64_smdpSigned2 = es9p_authenticate_client_resp.b64_smdp_signed_2;
    es10b_PrepareDownload_param.b64_smdpSignature2 = es9p_authenticate_client_resp.b64_smdp_signature_2;
    es10b_PrepareDownload_param.b64_smdpCertificate = es9p_authenticate_client_resp.b64_smdp_certificate;
    if (confirmation_code)
    {
        es10b_PrepareDownload_param.str_confirmationCode = confirmation_code;
    }
    else
    {
        es10b_PrepareDownload_param.str_confirmationCode = NULL;
    }

    jprint_progress("es10b_PrepareDownload");
    if (es10b_PrepareDownload(&euicc_ctx, &b64_prepare_download_response, &es10b_PrepareDownload_param))
    {
        jprint_error("es10b_PrepareDownload", NULL);
        return -1;
    }

    jprint_progress("es9p_get_bound_profile_package");
    if (es9p_get_bound_profile_package(&euicc_ctx, smdp, transaction_id, b64_prepare_download_response, &es9p_get_bound_profile_package_resp))
    {
        jprint_error("es9p_get_bound_profile_package", es9p_get_bound_profile_package_resp.status);
        return -1;
    }

    jprint_progress("es10b_LoadBoundProfilePackage");
    if (es10b_LoadBoundProfilePackage(&euicc_ctx, es9p_get_bound_profile_package_resp.b64_bpp))
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
