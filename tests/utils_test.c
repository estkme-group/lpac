#include <criterion/criterion.h>
#include <criterion/redirect.h>

#include <lpac/utils.h>

#include <limits.h>

Test(utils, str_to_bool_accepts_true_values) {
    cr_assert_eq(str_to_bool("y"), true);
    cr_assert_eq(str_to_bool("1"), true);
    cr_assert_eq(str_to_bool("YES"), true);
    cr_assert_eq(str_to_bool("on"), true);
}

Test(utils, str_to_bool_accepts_false_values) {
    cr_assert_eq(str_to_bool("n"), false);
    cr_assert_eq(str_to_bool("0"), false);
    cr_assert_eq(str_to_bool("No"), false);
    cr_assert_eq(str_to_bool("off"), false);
}

Test(utils, str_to_bool_rejects_unknown_values) { cr_assert_eq(str_to_bool("maybe"), -1); }

Test(utils, getenv_returns_default_when_unset) {
    unsetenv("LPAC_TEST_UTILS_UNSET");

    cr_assert_str_eq(getenv_str_or_default("LPAC_TEST_UTILS_UNSET", "fallback"), "fallback");
    cr_assert_eq(getenv_bool_or_default("LPAC_TEST_UTILS_UNSET", true), true);
    cr_assert_eq(getenv_int_or_default("LPAC_TEST_UTILS_UNSET", 17), 17);
    cr_assert_eq(getenv_long_or_default("LPAC_TEST_UTILS_UNSET", 23L), 23L);
}

Test(utils, getenv_parses_values) {
    setenv("LPAC_TEST_UTILS_VALUE", "false", 1);
    cr_assert_eq(getenv_bool_or_default("LPAC_TEST_UTILS_VALUE", true), false);

    setenv("LPAC_TEST_UTILS_VALUE", "42", 1);
    cr_assert_eq(getenv_int_or_default("LPAC_TEST_UTILS_VALUE", 0), 42);
    cr_assert_eq(getenv_long_or_default("LPAC_TEST_UTILS_VALUE", 0L), 42L);

    unsetenv("LPAC_TEST_UTILS_VALUE");
}

Test(utils, getenv_parses_negative_values) {
    setenv("LPAC_TEST_UTILS_VALUE", "-42", 1);

    cr_assert_eq(getenv_int_or_default("LPAC_TEST_UTILS_VALUE", 0), -42);
    cr_assert_eq(getenv_long_or_default("LPAC_TEST_UTILS_VALUE", 0L), -42L);

    unsetenv("LPAC_TEST_UTILS_VALUE");
}

Test(utils, getenv_parses_zero) {
    setenv("LPAC_TEST_UTILS_VALUE", "0", 1);

    cr_assert_eq(getenv_int_or_default("LPAC_TEST_UTILS_VALUE", 42), 0);
    cr_assert_eq(getenv_long_or_default("LPAC_TEST_UTILS_VALUE", 42L), 0L);

    unsetenv("LPAC_TEST_UTILS_VALUE");
}

Test(utils, getenv_long_saturates_positive_overflow) {
    setenv("LPAC_TEST_UTILS_VALUE", "999999999999999999999999999999", 1);

    cr_assert_eq(getenv_long_or_default("LPAC_TEST_UTILS_VALUE", 0L), LONG_MAX);

    unsetenv("LPAC_TEST_UTILS_VALUE");
}

Test(utils, getenv_long_saturates_negative_overflow) {
    setenv("LPAC_TEST_UTILS_VALUE", "-999999999999999999999999999999", 1);

    cr_assert_eq(getenv_long_or_default("LPAC_TEST_UTILS_VALUE", 0L), LONG_MIN);

    unsetenv("LPAC_TEST_UTILS_VALUE");
}

Test(utils, merge_array_of_str_preserves_array_order) {
    char *left[] = {"one", "two", NULL};
    char *right[] = {"three", NULL};
    char **result = merge_array_of_str(left, right);

    cr_assert_not_null(result);
    cr_assert_str_eq(result[0], "one");
    cr_assert_str_eq(result[1], "two");
    cr_assert_str_eq(result[2], "three");
    cr_assert_null(result[3]);
    free(result);
}

Test(utils, merge_array_of_str_handles_empty_arrays) {
    char *left[] = {NULL};
    char *right[] = {"one", NULL};
    char **result = merge_array_of_str(left, right);

    cr_assert_not_null(result);
    cr_assert_str_eq(result[0], "one");
    cr_assert_null(result[1]);
    free(result);
}

Test(utils, get_duration_handles_nanosecond_borrow) {
    const struct timespec start = {.tv_sec = 5, .tv_nsec = 900000000};
    const struct timespec end = {.tv_sec = 7, .tv_nsec = 100000000};
    const struct timespec duration = get_duration(start, end);

    cr_assert_eq(duration.tv_sec, 1);
    cr_assert_eq(duration.tv_nsec, 200000000);
}

Test(utils, get_duration_handles_end_before_start) {
    const struct timespec start = {.tv_sec = 5, .tv_nsec = 900000000};
    const struct timespec end = {.tv_sec = 4, .tv_nsec = 100000000};
    const struct timespec duration = get_duration(start, end);

    /* As C23, tv_nsec SHALL be positive number, so it can't be {-1, -800000000} */
    cr_assert_eq(duration.tv_sec, -2);
    cr_assert_eq(duration.tv_nsec, 200000000);
}
