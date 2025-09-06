#pragma once

#include <cjson/cJSON.h>
#include <euicc/es10b.h>
#include <euicc/es10c.h>
#include <euicc/es9p.h>

#include <stdbool.h>
#include <stdlib.h>

#define ENV_HTTP_DRIVER "LPAC_HTTP"
#define ENV_APDU_DRIVER "LPAC_APDU"

#define HTTP_ENV_NAME(DRIVER, NAME) ENV_HTTP_DRIVER "_" #DRIVER "_" #NAME
#define APDU_ENV_NAME(DRIVER, NAME) ENV_APDU_DRIVER "_" #DRIVER "_" #NAME
#define CUSTOM_ENV_NAME(NAME) "LPAC_CUSTOM_" #NAME

#define _cleanup_(x) __attribute__((cleanup(x)))

#define DEFINE_TRIVIAL_CLEANUP_FUNC(type, func) \
    static inline void func##p(type *p) {       \
        if (*p)                                 \
            func(*p);                           \
    }                                           \
    struct __useless_struct_to_allow_trailing_semicolon__

DEFINE_TRIVIAL_CLEANUP_FUNC(cJSON *, cJSON_Delete);
#define _cleanup_cjson_ _cleanup_(cJSON_Deletep)

DEFINE_TRIVIAL_CLEANUP_FUNC(char *, cJSON_free);
#define _cleanup_cjson_char_ _cleanup_(cJSON_freep)

DEFINE_TRIVIAL_CLEANUP_FUNC(struct es10b_notification_metadata_list *, es10b_notification_metadata_list_free_all);
#define _cleanup_es10b_notification_metadata_list_ _cleanup_(es10b_notification_metadata_list_free_allp)

DEFINE_TRIVIAL_CLEANUP_FUNC(struct es10b_rat *, es10b_rat_list_free_all);
#define _cleanup_es10b_rat_list_ _cleanup_(es10b_rat_list_free_allp)

DEFINE_TRIVIAL_CLEANUP_FUNC(struct es10c_profile_info_list *, es10c_profile_info_list_free_all);
#define _cleanup_es10c_profile_info_list_ _cleanup_(es10c_profile_info_list_free_allp)

DEFINE_TRIVIAL_CLEANUP_FUNC(char **, es11_smdp_list_free_all);
#define _cleanup_es11_smdp_list_ _cleanup_(es11_smdp_list_free_allp)

static inline void freep(void *p) { free(*(void **)p); }
#define _cleanup_free_ _cleanup_(freep)

#define getenv_or_default(name, default_value) \
    _Generic((default_value),                  \
        bool: getenv_bool_or_default,          \
        int: getenv_int_or_default,            \
        long: getenv_long_or_default,          \
        char *: getenv_str_or_default)(name, default_value)

const char *getenv_str_or_default(const char *name, const char *default_value);

bool getenv_bool_or_default(const char *name, bool default_value);

int getenv_int_or_default(const char *name, int default_value);

long getenv_long_or_default(const char *name, long default_value);

void set_deprecated_env_name(const char *name, const char *deprecated_name);

bool json_print(char *type, cJSON *jpayload);
