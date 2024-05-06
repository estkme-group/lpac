#include "discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <main.h>
#include <stdbool.h>

#include <euicc/es10a.h>
#include <euicc/es10b.h>
#include <euicc/es9p.h>

static const char *opt_string = "s:i:h?";

static const char *invalid_smds_list[] = {
    "testrootsmds.gsma.com",    // from SGP.26 v1.2
    "testrootsmds.example.com", // from SGP.26 v1.5
};

static char *known_smds_list[] = {
    "lpa.ds.gsma.com",            // GSM Association
    "lpa.live.esimdiscovery.com", // Google
    "lpads.vzw.otgeuicc.com",     // Verizon Wireless
};

static int applet_main(int argc, char **argv)
{
    int fret;

    int opt;

    char *smds = NULL;
    char *imei = NULL;

    char **smdp_list = NULL;
    char **smds_list = NULL;

    cJSON *jdata = NULL;

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
        default:
            break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    if (smds == NULL)
    {
        jprint_progress("es10a_get_euicc_configured_addresses", NULL);
        struct es10a_euicc_configured_addresses addresses;
        if (es10a_get_euicc_configured_addresses(&euicc_ctx, &addresses))
        {
            jprint_error("es10a_get_euicc_configured_addresses", NULL);
            goto err;
        }
        smds = strdup(addresses.rootDsAddress);
        if (is_invalid_smds_address(smds))
        {
            smds = NULL;
        }
    }

    if (smds != NULL)
    {
        smds_list = malloc(sizeof(char *));
        smds_list[0] = smds;
    }
    else
    {
        smds_list = known_smds_list;
    }

    jdata = cJSON_CreateArray();
    if (jdata == NULL)
    {
        goto err;
    }

    for (int i = 0; smds_list[i] != NULL; i++)
    {
        euicc_ctx.http.server_address = smds_list[i];

        jprint_progress("es10b_get_euicc_challenge_and_info", smds_list[i]);
        if (es10b_get_euicc_challenge_and_info(&euicc_ctx))
        {
            jprint_error("es10b_get_euicc_challenge_and_info", NULL);
            goto err;
        }

        jprint_progress("es9p_initiate_authentication", smds_list[i]);
        if (es9p_initiate_authentication(&euicc_ctx))
        {
            jprint_error("es9p_initiate_authentication", euicc_ctx.http.status.message);
            goto err;
        }

        jprint_progress("es10b_authenticate_server", smds_list[i]);
        if (es10b_authenticate_server(&euicc_ctx, NULL, imei))
        {
            jprint_error("es10b_authenticate_server", NULL);
            goto err;
        }

        jprint_progress("es11_authenticate_client", smds_list[i]);
        if (es11_authenticate_client(&euicc_ctx, &smdp_list))
        {
            jprint_error("es11_authenticate_client", NULL);
            goto err;
        }

        for (int j = 0; smdp_list[j] != NULL; j++)
        {
            cJSON *jsmdp = cJSON_CreateString(smdp_list[j]);
            if (jsmdp == NULL)
            {
                goto err;
            }
            cJSON_AddItemToArray(jdata, jsmdp);
        }
    }

    jprint_success(jdata);

    fret = 0;
    goto exit;

err:
    fret = -1;
exit:
    es11_smdp_list_free_all(smdp_list);
    es11_smdp_list_free_all(smds_list);
    euicc_http_cleanup(&euicc_ctx);
    return fret;
}

struct applet_entry applet_profile_discovery = {
    .name = "discovery",
    .main = applet_main,
};

static bool is_invalid_smds_address(const char *address)
{
    if (!is_valid_fqdn_name(address))
    {
        return true;
    }
    for (int i = 0; invalid_smds_list[i]; i++)
    {
        if (strncmp(address, invalid_smds_list[i], strlen(address)) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool is_valid_fqdn_name(const char *name) {
    int count = 0;
    int allowed;
    for (unsigned long i = strlen(name); i > 0; i--)
    {
        if (name[i] == '.')
        {
            if (count == 0)
            {
                return false;
            }
            count = 0;
            continue;
        }
        allowed = (name[i] >= '0' && name[i] <= '9') ||
                  (name[i] >= 'a' && name[i] <= 'z') ||
                  (name[i] >= 'A' && name[i] <= 'Z') ||
                  name[i] == '_' || name[i] == '-';
        if (!allowed || count > 63)
        {
            return false;
        }
        count++;
    }
    if (count > 63 || count == 0)
    {
        return false;
    }
    return true;
}
