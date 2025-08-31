#include "applet.h"

#include <stdio.h>
#include <string.h>

static void applet_usage(const char *selfname, const struct applet_entry **entries) {
    fprintf(stdout, "Usage: %s ", selfname);
    fputc('<', stdout);

    for (int i = 0; entries[i] != NULL; i++) {
        fputs(entries[i]->name, stdout);
        if (entries[i + 1] != NULL)
            fputc('|', stdout);
    }

    fputc('>', stdout);
    fputc('\n', stdout);
}

int applet_entry(const int argc, char **argv, const struct applet_entry **entries) {
    if (argc < 2) {
        applet_usage(argv[0], entries);
        return -1;
    }

    for (int i = 0; entries[i] != NULL; i++) {
        const struct applet_entry *entry = entries[i];
        if (strcmp(argv[1], entry->name) != 0)
            continue;
        return entry->main(argc - 1, argv + 1);
    }

    printf("Unknown command: %s\n", argv[1]);
    applet_usage(argv[0], entries);
    return -1;
}
