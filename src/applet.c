#include "applet.h"
#include <stdio.h>
#include <string.h>

static void applet_usage(const char *selfname, const struct applet_entry **entries)
{
    const struct applet_entry *entry;

    printf("Usage: %s <", selfname);

    while ((entry = *entries))
    {
        printf("%s", entry->name);
        if (*(++entries))
        {
            printf("|");
        }
    }

    printf(">\n");
}

int applet_entry(int argc, char **argv, const struct applet_entry **entries)
{
    const struct applet_entry **entries_cpy;
    const struct applet_entry *entry;

    entries_cpy = entries;

    if (argc < 2)
    {
        applet_usage(argv[0], entries);
        return -1;
    }

    while ((entry = *entries_cpy++))
    {
        if (strcmp(argv[1], entry->name) == 0)
        {
            return entry->main(argc - 1, argv + 1);
        }
    }

    printf("Unknown command: %s\n", argv[1]);
    applet_usage(argv[0], entries);
    return -1;
}
