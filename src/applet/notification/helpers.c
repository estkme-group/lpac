#include "helpers.h"

#include <ctype.h>
#include <string.h>

char *notification_strstrip(char *input) {
    if (input == NULL) return NULL;

    // Remove leading whitespace
    while (isspace((unsigned char) *input)) {
        input++;
    }

    // Remove trailing whitespace
    if (*input == '\0') return input;

    // Check if the string is not empty after leading whitespace removal
    char *end = input + strlen(input) - 1;
    while (end >= input && isspace((unsigned char) *end)) {
        *end = '\0';
        end--;
    }

    return input;
}
