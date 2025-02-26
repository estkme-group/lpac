#pragma once

#ifdef _WIN32
#define LINE_BREAK "\r\n"
#else
#define LINE_BREAK "\n"
#endif

#define fprintlnf(f, fmt, ...) fprintf(f, fmt LINE_BREAK, ## __VA_ARGS__); fflush(f)
#define eprintlnf(fmt, ...) fprintlnf(stderr, fmt LINE_BREAK, ## __VA_ARGS__)
#define printlnf(fmt, ...) fprintlnf(stdout, fmt, ## __VA_ARGS__)
