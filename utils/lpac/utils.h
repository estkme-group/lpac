#pragma once

#include <cjson/cJSON.h>
#include <euicc/es10b.h>
#include <euicc/es10c.h>
#include <euicc/es9p.h>

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define ENV_HTTP_DRIVER "LPAC_HTTP"
#define ENV_APDU_DRIVER "LPAC_APDU"

#define HTTP_ENV_NAME(DRIVER, NAME) ENV_HTTP_DRIVER "_" #DRIVER "_" #NAME
#define APDU_ENV_NAME(DRIVER, NAME) ENV_APDU_DRIVER "_" #DRIVER "_" #NAME
#define CUSTOM_ENV_NAME(NAME) "LPAC_CUSTOM_" #NAME

#if defined(_WIN32)
#    ifdef LIBLPAC_UTILS_EXPORTS
#        define LPAC_API __declspec(dllexport)
#    else
#        define LPAC_API __declspec(dllimport)
#    endif
#else
#    define LPAC_API __attribute__((visibility("default")))
#endif

#define _cleanup_(x) __attribute__((cleanup(x)))

#define DEFINE_TRIVIAL_CLEANUP_FUNC(type, func) \
    static inline void func##p(type *p) {       \
        if (*p)                                 \
            func(*p);                           \
    }                                           \
    struct __useless_struct_to_allow_trailing_semicolon__

DEFINE_TRIVIAL_CLEANUP_FUNC(cJSON *, cJSON_Delete);
#define _cleanup_cjson_ _cleanup_(cJSON_Deletep)

static inline void cJSON_freep(void *p) { cJSON_free(*(void **)p); }
#define _cleanup_cjson_free_ _cleanup_(cJSON_freep)

DEFINE_TRIVIAL_CLEANUP_FUNC(struct es10b_notification_metadata_list *, es10b_notification_metadata_list_free_all);
#define _cleanup_es10b_notification_metadata_list_ _cleanup_(es10b_notification_metadata_list_free_allp)

DEFINE_TRIVIAL_CLEANUP_FUNC(struct es10b_rat *, es10b_rat_list_free_all);
#define _cleanup_es10b_rat_list_ _cleanup_(es10b_rat_list_free_allp)

DEFINE_TRIVIAL_CLEANUP_FUNC(struct es10c_profile_info_list *, es10c_profile_info_list_free_all);
#define _cleanup_es10c_profile_info_list_ _cleanup_(es10c_profile_info_list_free_allp)

DEFINE_TRIVIAL_CLEANUP_FUNC(char **, es11_smdp_list_free_all);
#define _cleanup_es11_smdp_list_ _cleanup_(es11_smdp_list_free_allp)

DEFINE_TRIVIAL_CLEANUP_FUNC(DIR *, closedir);
#define _cleanup_dir_ _cleanup_(closedirp)

static inline void freep(void *p) { free(*(void **)p); }
#define _cleanup_free_ _cleanup_(freep)

#define getenv_or_default(name, default_value) \
    _Generic((default_value),                  \
        bool: getenv_bool_or_default,          \
        int: getenv_int_or_default,            \
        long: getenv_long_or_default,          \
        char *: getenv_str_or_default)(name, default_value)

LPAC_API const char *getenv_str_or_default(const char *name, const char *default_value);

LPAC_API bool getenv_bool_or_default(const char *name, bool default_value);

LPAC_API int getenv_int_or_default(const char *name, int default_value);

LPAC_API long getenv_long_or_default(const char *name, long default_value);

LPAC_API void set_deprecated_env_name(const char *name, const char *deprecated_name);

LPAC_API int str_to_bool(const char *value);

LPAC_API bool json_print(char *type, cJSON *jpayload);

LPAC_API bool ends_with(const char *restrict str, const char *restrict suffix);

LPAC_API char *remove_suffix(char *restrict str, const char *restrict suffix);

LPAC_API char **merge_array_of_str(char *left[], char *right[]);

LPAC_API char *path_concat(const char *restrict a, const char *restrict b);

LPAC_API struct timespec get_current_clock(clockid_t clock_id);

LPAC_API struct timespec get_duration(struct timespec t0, struct timespec t1);

LPAC_API struct timespec get_wall_time(struct timespec wall);
