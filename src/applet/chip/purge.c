#include "applet.h"

#include "main.h"

#include <euicc/es10c.h>

#include <stdio.h>
#include <string.h>

int applet_chip_purge_main(const int argc, char **argv) {
    int ret;

    if (argc < 2) {
        printf("Usage: %s [yes|other]\n", argv[0]);
        printf("\t\tConfirm purge eUICC, all data will lost!\n");
        return -1;
    }

    if (strcmp(argv[1], "yes") != 0) {
        printf("Purge canceled\n");
        return -1;
    }

    if ((ret = es10c_euicc_memory_reset(&euicc_ctx))) {
        const char *reason;
        switch (ret) {
        case 1:
            reason = "nothing to delete";
            break;
        default:
            reason = "unknown";
            break;
        }
        jprint_error("es10c_euicc_memory_reset", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}
