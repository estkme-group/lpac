#pragma once

struct es9p_error {
    const char *subject_code;
    const char *reason_code;
    const char *description;
};

const char* es9p_error_message(const char *subject_code, const char *reason_code);
