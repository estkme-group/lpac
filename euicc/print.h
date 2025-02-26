#pragma once

#define fprintlnf(f, fmt, ...) fprintf(f, fmt "\n", ## __VA_ARGS__); fflush(f)
#define eprintlnf(fmt, ...) fprintlnf(stderr, fmt, ## __VA_ARGS__)
#define printlnf(fmt, ...) fprintlnf(stdout, fmt, ## __VA_ARGS__)
