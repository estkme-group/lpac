#include "applet.h"
#include <driver.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lpac/utils.h>
#include <main.h>

static void applet_usage(const char *selfname, const struct applet_entry **entries) {
    const struct applet_entry *entry;

    printf("Usage: %s <", selfname);

    while ((entry = *entries)) {
        printf("%s", entry->name);
        if (*(++entries)) {
            printf("|");
        }
    }

    printf(">\n");
}

int applet_entry(int argc, char **argv, const struct applet_entry **entries) {
    const struct applet_entry **ptr = entries;
    const struct applet_entry *entry;

    if (argc < 2) {
        applet_usage(argv[0], entries);
        return -1;
    }

    while ((entry = *ptr++)) {
        if (strcmp(argv[1], entry->name) == 0) {
            if (entry->subapplets && argc > 2) {
                return applet_entry(argc - 1, argv + 1, entry->subapplets);
            }

            if (entry->main) {
                if (!entry->skip_init_driver) {
                    if (euicc_driver_init(getenv(ENV_APDU_DRIVER), getenv(ENV_HTTP_DRIVER))) {
                        fprintf(stderr, "euicc_driver_init failed\n");
                        return -1;
                    }
                }
                if (!entry->skip_init_euicc) {
                    const int ret = main_init_euicc();
                    if (ret != 0)
                        return ret;
                }
                return entry->main(argc - 1, argv + 1);
            }

            applet_usage(argv[0], entry->subapplets ? entry->subapplets : entries);
            return -1;
        }
    }

    printf("Unknown command: %s\n", argv[1]);
    applet_usage(argv[0], entries);
    return -1;
}
