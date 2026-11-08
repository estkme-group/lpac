#include "at_cmd.h"

#include <stdlib.h>
#include <string.h>

int at_pop_line(char *buf, size_t *buf_len, char *line, size_t line_size) {
    char *newline = memchr(buf, '\n', *buf_len);
    if (!newline)
        return -1;

    const size_t consumed = newline - buf + 1;
    size_t line_len = consumed - 1;
    if (line_len >= line_size)
        line_len = line_size - 1;
    memcpy(line, buf, line_len);
    line[line_len] = '\0';

    memmove(buf, buf + consumed, *buf_len - consumed);
    *buf_len -= consumed;

    line[strcspn(line, "\r")] = 0;
    return 0;
}

int at_match_line(const char *line, const char *expected, char **found_response_data, int *result) {
    if (strlen(line) == 0)
        return -1;

    if (strcmp(line, "ERROR") == 0) {
        *result = -1;
        return 0;
    }
    if (strcmp(line, "OK") == 0) {
        *result = 0;
        return 0;
    }

    if (expected && strncmp(line, expected, strlen(expected)) == 0) {
        free(*found_response_data);
        *found_response_data = strdup(line + strlen(expected));
        while (*found_response_data && (*found_response_data)[0] == ' ')
            memmove(*found_response_data, *found_response_data + 1, strlen(*found_response_data));
    }
    return -1;
}

int at_expect(struct at_userdata *userdata, char **response, const char *expected) {
    return at_expect_with_deadline(userdata, response, expected, AT_DEADLINE_INFINITE);
}
