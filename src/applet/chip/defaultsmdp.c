#include "defaultsmdp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10a.h>

static int applet_main(int argc, char **argv)
{
    const char *smdp;

    if (argc < 2)
    {
        printf("Usage: %s <smdp>\n", argv[0]);
        return -1;
    }

    smdp = argv[1];

    if (es10a_set_default_dp_address(&euicc_ctx, smdp))
    {
        jprint_error("es10a_set_default_dp_address", NULL);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_chip_defaultsmdp = {
    .name = "defaultsmdp",
    .main = applet_main,
};
