#include "applet.h"

#include "main.h"

#include <euicc/es10a.h>

#include <stdio.h>

int applet_chip_default_smdp_main(int argc, char **argv) {
    const char *smdp;

    if (argc < 2) {
        printf("Usage: %s <smdp>\n", argv[0]);
        return -1;
    }

    smdp = argv[1];

    if (es10a_set_default_dp_address(&euicc_ctx, smdp)) {
        jprint_error("es10a_set_default_dp_address", NULL);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}
