#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

static int main(int argc, char **argv)
{
    char *transaction_id = NULL;

    char *smdp = NULL;
    char *matchingId = NULL;
    char *imei = NULL;
    char *confirmation_code = NULL;

    char *b64_euicc_challenge = NULL;
    char *b64_euicc_info_1 = NULL;
    struct es9p_initiate_authentication_resp es9p_initiate_authentication_resp;

    struct es10b_authenticate_server_param es10b_authenticate_server_param;
    char *b64_authenticate_server_response = NULL;

    struct es9p_authenticate_client_resp es9p_authenticate_client_resp;

    struct es10b_prepare_download_param es10b_prepare_download_param;
    char *b64_prepare_download_response = NULL;

    struct es9p_get_bound_profile_package_resp es9p_get_bound_profile_package_resp;

    smdp = getenv("SMDP");
    matchingId = getenv("MATCHINGID");
    imei = getenv("IMEI");
    confirmation_code = getenv("CONFIRMATION_CODE");

    if (smdp == NULL)
    {
        if (es10a_get_euicc_configured_addresses(&euicc_ctx, &smdp, NULL))
        {
            jprint_error("es10a_get_euicc_configured_addresses", NULL);
            return -1;
        }
    }

    if (!smdp || (strlen(smdp) == 0))
    {
        jprint_error("smdp is null", NULL);
        return -1;
    }

    if (es10b_get_euicc_challenge(&euicc_ctx, &b64_euicc_challenge))
    {
        jprint_error("es10b_get_euicc_challenge", NULL);
        return -1;
    }

    if (es10b_get_euicc_info(&euicc_ctx, &b64_euicc_info_1))
    {
        jprint_error("es10b_get_euicc_info", NULL);
        return -1;
    }

    if (es9p_initiate_authentication(&euicc_ctx, smdp, b64_euicc_challenge, b64_euicc_info_1, &es9p_initiate_authentication_resp))
    {
        jprint_error("es9p_initiate_authentication", es9p_initiate_authentication_resp.status);
        return -1;
    }

    transaction_id = strdup(es9p_initiate_authentication_resp.transaction_id);
    es10b_authenticate_server_param.b64_server_signed_1 = es9p_initiate_authentication_resp.b64_server_signed_1;
    es10b_authenticate_server_param.b64_server_signature_1 = es9p_initiate_authentication_resp.b64_server_signature_1;
    es10b_authenticate_server_param.b64_euicc_ci_pkid_to_be_used = es9p_initiate_authentication_resp.b64_euicc_ci_pkid_to_be_used;
    es10b_authenticate_server_param.b64_server_certificate = es9p_initiate_authentication_resp.b64_server_certificate;
    es10b_authenticate_server_param.matchingId = matchingId;
    es10b_authenticate_server_param.imei = imei;
    es10b_authenticate_server_param.tac = NULL;

    if (es10b_authenticate_server(&euicc_ctx, &b64_authenticate_server_response, &es10b_authenticate_server_param))
    {
        jprint_error("es10b_authenticate_server", NULL);
        return -1;
    }

    if (es9p_authenticate_client(&euicc_ctx, smdp, transaction_id, b64_authenticate_server_response, &es9p_authenticate_client_resp))
    {
        jprint_error("es9p_authenticate_client", es9p_authenticate_client_resp.status);
        return -1;
    }

    es10b_prepare_download_param.b64_smdp_signed_2 = es9p_authenticate_client_resp.b64_smdp_signed_2;
    es10b_prepare_download_param.b64_smdp_signature_2 = es9p_authenticate_client_resp.b64_smdp_signature_2;
    es10b_prepare_download_param.b64_smdp_certificate = es9p_authenticate_client_resp.b64_smdp_certificate;
    if (confirmation_code)
    {
        es10b_prepare_download_param.str_checkcode = confirmation_code;
        es10b_prepare_download_param.hexstr_transcation_id = transaction_id;
    }
    else
    {
        es10b_prepare_download_param.str_checkcode = NULL;
    }

    if (es10b_prepare_download(&euicc_ctx, &b64_prepare_download_response, &es10b_prepare_download_param))
    {
        jprint_error("es10b_prepare_download", NULL);
        return -1;
    }

    if (es9p_get_bound_profile_package(&euicc_ctx, smdp, transaction_id, b64_prepare_download_response, &es9p_get_bound_profile_package_resp))
    {
        jprint_error("es9p_get_bound_profile_package", es9p_get_bound_profile_package_resp.status);
        return -1;
    }

    if (es10b_load_bound_profile_package(&euicc_ctx, es9p_get_bound_profile_package_resp.b64_bpp))
    {
        jprint_error("es10b_load_bound_profile_package", NULL);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_profile_download = {
    .name = "download",
    .main = main,
};
