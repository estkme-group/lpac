#include "applet.h"

#include "main.h"

#include <euicc/es10b.h>
#include <euicc/es9p.h>
#include <lpac/utils.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *opt_string = "s:i:h?";

static int applet_main(int argc, char **argv) {
    int fret;

    int opt;

    char *smds = NULL;
    char *imei = NULL;

    _cleanup_es11_smdp_list_ char **smdp_list = NULL;

    cJSON *jdata = NULL;

    while ((opt = getopt(argc, argv, opt_string)) != -1) {
        switch (opt) {
        case 's':
            smds = strdup(optarg);
            break;
        case 'i':
            imei = strdup(optarg);
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("\t -s SM-DS Domain\n");
            printf("\t -i IMEI\n");
            printf("\t -h This help info\n");
            return -1;
            break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    if (smds == NULL) {
        // smds = "prod.smds.rsp.goog";
        // smds = "lpa.live.esimdiscovery.com";
        smds = "lpa.ds.gsma.com";
    }

    euicc_ctx.http.server_address = smds;

    jprint_progress("es10b_get_euicc_challenge_and_info", smds);
    if (es10b_get_euicc_challenge_and_info(&euicc_ctx)) {
        jprint_error("es10b_get_euicc_challenge_and_info", NULL);
        goto err;
    }

    jprint_progress("es9p_initiate_authentication", smds);
    if (es9p_initiate_authentication(&euicc_ctx)) {
        jprint_error("es9p_initiate_authentication", euicc_ctx.http.status.message);
        goto err;
    }

    jprint_progress("es10b_authenticate_server", smds);
    if (es10b_authenticate_server(&euicc_ctx, NULL, imei)) {
        jprint_error("es10b_authenticate_server", NULL);
        goto err;
    }

    jprint_progress("es11_authenticate_client", smds);
    if (es11_authenticate_client(&euicc_ctx, &smdp_list)) {
        jprint_error("es11_authenticate_client", NULL);
        goto err;
    }

    jdata = cJSON_CreateArray();
    if (jdata == NULL) {
        goto err;
    }

    for (int i = 0; smdp_list[i] != NULL; i++) {
        cJSON *jsmdp = cJSON_CreateString(smdp_list[i]);
        if (jsmdp == NULL) {
            goto err;
        }
        cJSON_AddItemToArray(jdata, jsmdp);
    }

    jprint_success(jdata);

    fret = 0;
    goto exit;

err:
    fret = -1;
exit:
    euicc_http_cleanup(&euicc_ctx);
    return fret;
}

struct applet_entry applet_profile_discovery = {
    .name = "discovery",
    .main = applet_main,
};
