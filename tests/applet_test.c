#include <criterion/criterion.h>

#include <applet.h>

Test(lpac, rejects_missing_command) {
    const struct applet_entry *entries[] = {NULL};
    char *argv[] = {"lpac", NULL};

    cr_assert_eq(applet_entry(1, argv, entries), -1);
}

Test(lpac, rejects_unknown_command) {
    const struct applet_entry *entries[] = {NULL};
    char *argv[] = {"lpac", "unknown", NULL};

    cr_assert_eq(applet_entry(2, argv, entries), -1);
}
