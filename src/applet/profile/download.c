#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <main.h>
#include <signal.h>

#include <euicc/es10a.h>
#include <euicc/es10b.h>
#include <euicc/es9p.h>
#include <euicc/es8p.h>
#include <euicc/tostr.h>

static const char *opt_string = "s:m:i:c:a:ph?";

static volatile int cancelled = 0;

#define CANCELPOINT() \
    if (cancelled)    \
    {                 \
        goto err;     \
    }

static void sigint_handler(int x)
{
    cancelled = 1;
}

static int applet_main(int argc, char **argv)
{
    int fret;
    const char *error_function_name = NULL;
    const char *error_detail = NULL;

    int opt;

    char *smdp = NULL;
    char *matchingId = NULL;
    char *imei = NULL;
    char *confirmation_code = NULL;
    char *activation_code = NULL;
    int interactive_preview = 0;

    struct es10a_euicc_configured_addresses configured_addresses = {0};
    struct es10b_load_bound_profile_package_result download_result = {0};

    cJSON *jmetadata = NULL;
    struct es8p_metadata *profile_metadata = NULL;

    while ((opt = getopt(argc, argv, opt_string)) != -1)
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
        case 'p':
            interactive_preview = 1;
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS]\r\n", argv[0]);
            printf("\t -s SM-DP+ Domain\r\n");
            printf("\t -m Matching ID\r\n");
            printf("\t -i IMEI\r\n");
            printf("\t -c Confirmation Code (Password)\r\n");
            printf("\t -a Activation Code (e.g: 'LPA:***')\r\n");
            printf("\t -p Interactive preview profile\r\n");
            printf("\t -h This help info\r\n");
            return -1;
        default:
            break;
        }
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
                    error_function_name = "activation_code";
                    error_detail = "invalid";
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
                    error_function_name = "confirmation_code";
                    error_detail = "required";
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
            error_function_name = "es10a_get_euicc_configured_addresses";
            error_detail = NULL;
            goto err;
        }
        else
        {
            smdp = configured_addresses.defaultDpAddress;
        }
    }

    if (!smdp || (strlen(smdp) == 0))
    {
        error_function_name = "smdp";
        error_detail = "empty";
        goto err;
    }

    signal(SIGINT, sigint_handler);

    euicc_ctx.http.server_address = smdp;

    CANCELPOINT();
    jprint_progress("es10b_get_euicc_challenge_and_info", smdp);
    if (es10b_get_euicc_challenge_and_info(&euicc_ctx))
    {
        error_function_name = "es10b_get_euicc_challenge_and_info";
        error_detail = NULL;
        goto err;
    }

    CANCELPOINT();
    jprint_progress("es9p_initiate_authentication", smdp);
    if (es9p_initiate_authentication(&euicc_ctx))
    {
        error_function_name = "es9p_initiate_authentication";
        error_detail = euicc_ctx.http.status.message;
        goto err;
    }

    CANCELPOINT();
    jprint_progress("es10b_authenticate_server", smdp);
    if (es10b_authenticate_server(&euicc_ctx, matchingId, imei))
    {
        error_function_name = "es10b_authenticate_server";
        error_detail = NULL;
        goto err;
    }

    CANCELPOINT();
    jprint_progress("es9p_authenticate_client", smdp);
    if (es9p_authenticate_client(&euicc_ctx))
    {
        error_function_name = "es9p_authenticate_client";
        error_detail = euicc_ctx.http.status.message;
        goto err;
    }

    // preview here
    if (euicc_ctx.http._internal.prepare_download_param->b64_profileMetadata)
    {
        CANCELPOINT();
        if (es8p_metadata_parse(&profile_metadata, euicc_ctx.http._internal.prepare_download_param->b64_profileMetadata))
        {
            error_function_name = "es8p_meatadata_parse";
            error_detail = NULL;
            goto err;
        }

        jmetadata = cJSON_CreateObject();

        cJSON_AddStringOrNullToObject(jmetadata, "iccid", profile_metadata->iccid);
        cJSON_AddStringOrNullToObject(jmetadata, "serviceProviderName", profile_metadata->serviceProviderName);
        cJSON_AddStringOrNullToObject(jmetadata, "profileName", profile_metadata->profileName);
        cJSON_AddStringOrNullToObject(jmetadata, "iconType", euicc_icontype2str(profile_metadata->iconType));
        cJSON_AddStringOrNullToObject(jmetadata, "icon", profile_metadata->icon);
        cJSON_AddStringOrNullToObject(jmetadata, "profileClass", euicc_profileclass2str(profile_metadata->profileClass));

        jprint_progress_obj("es8p_meatadata_parse", jmetadata);

        if (interactive_preview)
        {
            char c;
            jprint_progress("preview", "y/n");
            c = getchar();
            if (c != 'y' && c != 'Y')
            {
                cancelled = 1;
            }
        }
    }

    CANCELPOINT();
    jprint_progress("es10b_prepare_download", smdp);
    if (es10b_prepare_download(&euicc_ctx, confirmation_code))
    {
        error_function_name = "es10b_prepare_download";
        error_detail = NULL;
        goto err;
    }

    CANCELPOINT();
    jprint_progress("es9p_get_bound_profile_package", smdp);
    if (es9p_get_bound_profile_package(&euicc_ctx))
    {
        error_function_name = "es9p_get_bound_profile_package";
        error_detail = euicc_ctx.http.status.message;
        goto err;
    }

    CANCELPOINT();
    jprint_progress("es10b_load_bound_profile_package", smdp);
    if (es10b_load_bound_profile_package(&euicc_ctx, &download_result))
    {
        char buffer[256];

        snprintf(buffer, sizeof(buffer), "%s,%s", euicc_bppcommandid2str(download_result.bppCommandId), euicc_errorreason2str(download_result.errorReason));
        error_function_name = "es10b_load_bound_profile_package";
        error_detail = buffer;

        goto err;
    }

    jprint_success(NULL);

    fret = 0;
    goto exit;

err:
    fret = -1;
    if (error_detail)
    {
        error_detail = strdup(error_detail);
    }
    jprint_progress("es10b_cancel_session", smdp);
    es10b_cancel_session(&euicc_ctx, ES10B_CANCEL_SESSION_REASON_ENDUSERREJECTION);
    jprint_progress("es9p_cancel_session", smdp);
    es9p_cancel_session(&euicc_ctx);
    if (!cancelled)
    {
        jprint_error(error_function_name, error_detail);
    }
    else
    {
        jprint_error("cancelled", NULL);
    }
    free((void *)error_detail);
exit:
    es8p_metadata_free(&profile_metadata);
    es10a_euicc_configured_addresses_free(&configured_addresses);
    euicc_http_cleanup(&euicc_ctx);
    return fret;
}

struct applet_entry applet_profile_download = {
    .name = "download",
    .main = applet_main,
};
