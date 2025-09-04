#include "applet.h"

#include "main.h"

#include <euicc/es10c.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *build_reason(const int reason, const char *argv0) {
    switch (reason) {
    case 1:
        return "iccid or aid not found";
    case 2:
        if (strcmp(argv0, "disable") == 0)
            return "profile not in enabled state";
        return "profile not in disabled state";
    case 3:
        return "disallowed by policy";
    case 4: // enable only
        return "wrong profile re-enabling";
    case -1:
        return "internal error, maybe illegal iccid/aid coding";
    default:
        return "unknown";
    }
}

static void jprint_reason(const int reason, char *argv0) {
    const int n = snprintf(NULL, 0, "es10c_%s_profile", argv0);
    char method[n];
    snprintf(method, n, "es10c_%s_profile", argv0);
    jprint_error(method, build_reason(reason, argv0));
}

int profile_operation(const int argc, char **argv) {
    if (strcmp(argv[0], "delete") == 0)
        return es10c_delete_profile(&euicc_ctx, argv[1]);
    const bool refresh = argc > 2 ? strtol(argv[2], NULL, 10) != 0 : false;
    if (strcmp(argv[0], "enable") == 0)
        return es10c_enable_profile(&euicc_ctx, argv[1], refresh);
    if (strcmp(argv[0], "disable") == 0)
        return es10c_disable_profile(&euicc_ctx, argv[1], refresh);
    return -1;
}

int applet_profile_operation_main(const int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s [iccid/aid]", argv[0]);
        if (strcmp(argv[0], "delete") != 0)
            printf(" [refresh-flag]\n\t[refresh-flag]: optional");
        printf("\n");
        return -1;
    }

    const int reason = profile_operation(argc, argv);
    if (reason == 0) {
        jprint_success(NULL);
        return 0;
    }
    jprint_reason(reason, argv[0]);
    return -1;
}
