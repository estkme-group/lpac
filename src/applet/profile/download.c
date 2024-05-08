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
#include <euicc/tostr.h>

static const char *opt_string = "s:m:i:c:a:h?";

static int applet_main(int argc, char **argv)
{
    int fret;

    int opt;

    char *smdp = NULL;
    char *matchingId = NULL;
    char *imei = NULL;
    char *confirmation_code = NULL;
    char *activation_code = NULL;

    struct es10a_euicc_configured_addresses configured_addresses = {0};
    struct es10b_load_bound_profile_package_result download_result = {0};

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
        case 'a':
            activation_code = strdup(optarg);
            if (strncmp(activation_code, "LPA:", 4) == 0)
            {
                activation_code += 4; // ignore uri scheme
            }
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS]\r\n", argv[0]);
            printf("\t -s SM-DP+ Domain\r\n");
            printf("\t -m Matching ID\r\n");
            printf("\t -i IMEI\r\n");
            printf("\t -c Confirmation Code (Password)\r\n");
            printf("\t -a Activation Code (e.g: 'LPA:***')\r\n");
            printf("\t -h This help info\r\n");
            return -1;
        default:
            break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    if (activation_code != NULL)
    {
        // SGP.22 v2.2.2; Page 111
        // Section: 4.1 (Activation Code)

        char *token = NULL;
        int index = 0;

        for (token = strtok(activation_code, "$"); token != NULL; token = strtok(NULL, "$"))
        {
            switch (index)
            {
            case 0: // Activation Code Format
                if (strncmp(token, "1", strlen(token)) != 0)
                {
                    jprint_error("invalid activation code format", NULL);
                    goto err;
                }
                break;
            case 1: // SM-DP+ Address
                smdp = strdup(token);
                break;
            case 2: // AC_Token or Matching ID
                matchingId = strdup(token);
                break;
            case 3: // SM-DP+ OID
                // ignored; this function is not implemented
                break;
            case 4: // Confirmation Code Required Flag
                if (strncmp(token, "1", strlen(token)) == 0 && confirmation_code == NULL)
                {
                    jprint_error("confirmation code required", NULL);
                    goto err;
                }
                break;
            default:
                break;
            }
            index++;
        }
    }

    if (smdp == NULL)
    {
        jprint_progress("es10a_get_euicc_configured_addresses", NULL);
        if (es10a_get_euicc_configured_addresses(&euicc_ctx, &configured_addresses))
        {
            jprint_error("es10a_get_euicc_configured_addresses", NULL);
            goto err;
        }
        else
        {
            smdp = configured_addresses.defaultDpAddress;
        }
    }

    if (!smdp || (strlen(smdp) == 0))
    {
        jprint_error("smdp is null", NULL);
        goto err;
    }

    euicc_ctx.http.server_address = smdp;

    jprint_progress("es10b_get_euicc_challenge_and_info", smdp);
    if (es10b_get_euicc_challenge_and_info(&euicc_ctx))
    {
        jprint_error("es10b_get_euicc_challenge_and_info", NULL);
        goto err;
    }

    jprint_progress("es9p_initiate_authentication", smdp);
    if (es9p_initiate_authentication(&euicc_ctx))
    {
        jprint_error("es9p_initiate_authentication", euicc_ctx.http.status.message);
        goto err;
    }

    jprint_progress("es10b_authenticate_server", smdp);
    if (es10b_authenticate_server(&euicc_ctx, matchingId, imei))
    {
        jprint_error("es10b_authenticate_server", NULL);
        goto err;
    }

    jprint_progress("es9p_authenticate_client", smdp);
    if (es9p_authenticate_client(&euicc_ctx))
    {
        jprint_error("es9p_authenticate_client", euicc_ctx.http.status.message);
        goto err;
    }

    jprint_progress("es10b_prepare_download", smdp);
    if (es10b_prepare_download(&euicc_ctx, confirmation_code))
    {
        jprint_error("es10b_prepare_download", NULL);
        goto err;
    }

    jprint_progress("es9p_get_bound_profile_package", smdp);
    if (es9p_get_bound_profile_package(&euicc_ctx))
    {
        jprint_error("es9p_get_bound_profile_package", euicc_ctx.http.status.message);
        goto err;
    }

    jprint_progress("es10b_load_bound_profile_package", smdp);
    if (es10b_load_bound_profile_package(&euicc_ctx, &download_result))
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s,%s", euicc_bppcommandid2str(download_result.bppCommandId), euicc_errorreason2str(download_result.errorReason));
        jprint_error("es10b_load_bound_profile_package", buffer);
        goto err;
    }

    jprint_success(NULL);

    fret = 0;
    goto exit;

err:
    fret = -1;
exit:
    es10a_euicc_configured_addresses_free(&configured_addresses);
    euicc_http_cleanup(&euicc_ctx);
    return fret;
}

struct applet_entry applet_profile_download = {
    .name = "download",
    .main = applet_main,
};
