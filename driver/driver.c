// Needed by dlinfo(3).
#define _GNU_SOURCE
#include "driver.h"
#include "driver.private.h"

#include <lpac/list.h>
#include <lpac/utils.h>
#if defined(_WIN32)
#    include <dlfcn-win32/dlfcn.h>
#endif

#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__linux__)
#    include <dlfcn.h>
#    include <elf.h>
#    include <libgen.h>
#    include <link.h>
#    include <linux/limits.h>
#elif defined(__FreeBSD__)
#    include <sys/sysctl.h>
#    include <sys/syslimits.h>
#elif defined(__APPLE__) && defined(__MACH__)
#    include <dlfcn.h>
#    include <mach-o/dyld.h>
#    include <sys/syslimits.h>
#elif defined(_WIN32)
#    include <errhandlingapi.h>
#    include <libloaderapi.h>
#    include <winerror.h>
#endif

#include "driver/apdu/stdio.h"
#include "driver/http/stdio.h"

static const struct euicc_driver *builtin_drivers[] = {
    &driver_apdu_stdio,
    &driver_http_stdio,
    NULL,
};

static const struct euicc_driver *_driver_apdu = NULL;
static const struct euicc_driver *_driver_http = NULL;

struct euicc_apdu_interface euicc_driver_interface_apdu;
struct euicc_http_interface euicc_driver_interface_http;

struct euicc_drivers_list {
    const struct euicc_driver *driver;
    struct list_head list;
};

static LIST_HEAD(drivers);

#if defined(_WIN32)
static const char *dynlib_suffix = ".dll";
#elif defined(__APPLE__) && defined(__MACH__)
static const char *dynlib_suffix = ".dylib";
#elif defined(__unix__) || defined(__unix)
static const char *dynlib_suffix = ".so";
#else
#    error "Unsupported platform."
#endif

#if defined(__unix__) || defined(__unix)
static char *get_origin() {
#    if defined(__FreeBSD__)
    int mib[4] = {[0] = CTL_KERN, [1] = KERN_PROC, [2] = KERN_PROC_PATHNAME, [3] = -1};
    size_t size;
    if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0) {
        goto fallback;
    }
    char *buf = (char *)calloc(size + 1, sizeof(char));
    if (sysctl(mib, 4, buf, &size, NULL, 0) == 0) {
        return buf;
    }
    free(buf);
#    endif
    char buf[PATH_MAX] = {0};
#    if defined(__linux__)
    const char *self_path = "/proc/self/exe";
#    elif defined(__FreeBSD__)
    const char *self_path = "/proc/curproc/file";
#    endif
    if (readlink(self_path, buf, sizeof(buf) - 1) < 0) {
        return NULL;
    }
    return strdup(dirname(buf));
}
#elif defined(_WIN32)
static char *get_origin() {
    size_t size = MAX_PATH;
    char *buf = (char *)calloc(size, sizeof(char));
    int res = GetModuleFileNameA(NULL, buf, MAX_PATH);
    while (res == MAX_PATH && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        size *= 2;
        char *tmp = (char *)realloc(buf, size * sizeof(char));
        if (tmp == NULL) {
            return NULL;
        }
        buf = tmp;
        res = GetModuleFileNameA(NULL, buf, MAX_PATH);
    };
    if (res == 0) {
        fprintf(stderr, "Failed to get $ORIGIN.\n");
        return NULL;
    }
    char *last_slash = strrchr(buf, '\\');
    if (last_slash)
        *last_slash = '\0';
    return buf;
}
#elif defined(__APPLE__) && defined(__MACH__)
static char *get_origin() {
    char *buf = (char *)calloc(PATH_MAX, sizeof(char));
    uint32_t size = PATH_MAX;
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return buf;
    } else {
        return NULL;
    }
}
#endif

/*
 * Glibc, Musl, FreeBSD and Soloris support RTLD_DI_LINKMAP.
 * Musl hate the vendor detect macro and refuse to support __musl__.
 * They think we should test feature instead of detect vendor, it's normally right,
 * but for dlinfo(3), it's almost impossible because dlinfo has no errno or similar thing,
 * there is dlerror(3), right? No! dlerror(3) can only return a implementation-defined error string
 * (In C term, it should be called unspecified behavior because libc implementation needn't have
 * documentation for it) instead of an integer which we can compare it with specific value
 * programmatically. So we can't test this feature is available, we can only assume there is only
 * Glibc and Musl on Linux, forget Nolibc and other libc implementations on Linux.
 */
#if defined(__GLIBC__) || defined(__linux__) || defined(__FreeBSD__) || (defined(__sun) && defined(__SVR4))
static char *get_runpath() {
    void *handle = dlopen(NULL, RTLD_NOW);
    if (handle == NULL) {
        return NULL;
    }
    struct link_map *linkmap;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &linkmap) == -1) {
        return NULL;
    }
    const ElfW(Dyn) *dyn = linkmap->l_ld;
    const ElfW(Dyn) *rpath = NULL;
    const ElfW(Dyn) *runpath = NULL;
    const char *strtab = NULL;
    for (; dyn->d_tag != DT_NULL; ++dyn) {
        if (dyn->d_tag == DT_RPATH) {
            rpath = dyn;
        } else if (dyn->d_tag == DT_RUNPATH) {
            runpath = dyn;
        } else if (dyn->d_tag == DT_STRTAB) {
            strtab = (const char *)dyn->d_un.d_ptr;
        }
    }
    if (runpath != NULL && strtab != NULL) {
        return strdup(strtab + runpath->d_un.d_val);
    } else {
        return NULL;
    }
}
// If DT_RUNPATH is not accessable, use the first search path as alternative.
#elif defined(__unix__) || defined(__unix)
static char *get_runpath() {
    void *handle = dlopen(NULL, RTLD_NOW);
    if (handle == NULL) {
        return NULL;
    }
    Dl_serinfo serinfo;
    if (dlinfo(handle, RTLD_DI_SERINFOSIZE, &serinfo) == -1) {
        return NULL;
    }
    _cleanup_free_ Dl_serinfo *sip = (Dl_serinfo *)malloc(serinfo.dls_size);
    if (dlinfo(handle, RTLD_DI_SERINFOSIZE, sip) == -1) {
        return NULL;
    }
    if (dlinfo(handle, RTLD_DI_SERINFO, sip) == -1) {
        return NULL;
    }
    size_t len = strlen(sip->dls_serpath[0].dls_name);
    char *runpath = strdup(sip->dls_serpath[0].dls_name);
    for (size_t i = 1; i < serinfo.dls_cnt; i++) {
        len = len + 1 /* SEP */ + strlen(sip->dls_serpath[i].dls_name) + 1;
        char *tmp = realloc(runpath, len * sizeof(char));
        if (tmp == NULL) {
            return NULL;
        }
        strcat(tmp, ":");
        strcat(tmp, sip->dls_serpath[i].dls_name);
        runpath = tmp;
    }
    return runpath;
}
#else
static char *get_runpath() { return strdup("$ORIGIN"); }
#endif

static char *get_first_runpath() {
    _cleanup_free_ char *runpath = get_runpath();
    if (runpath == NULL)
        return NULL;
    // runpath is not empty, so strtok shouldn't return NULL when first call.
    char *tmp = strtok(runpath, ":");
    if (!strcmp(tmp, "$ORIGIN") || !strcmp(tmp, "@loader_path")) {
        return get_origin();
    } else {
        return strdup(tmp);
    }
}

static char *get_driver_path() {
    _cleanup_free_ char *LPAC_DRIVER_HOME = get_first_runpath();
    if (LPAC_DRIVER_HOME == NULL)
        return NULL;
    return path_concat(LPAC_DRIVER_HOME, "driver");
}

static const struct euicc_driver *find_driver_by_path(const char *restrict dir, char *restrict name) {
    _cleanup_free_ char *driver_path = path_concat(dir, name);
    if (access(driver_path, R_OK) != 0) {
        return NULL;
    }
    void *handle = dlopen(driver_path, RTLD_NOW);
    if (handle == NULL) {
        return NULL;
    }
    _cleanup_free_ char *ifn = remove_suffix(name, dynlib_suffix);
    if (ifn == NULL) {
        return NULL;
    }
    struct euicc_driver *driver = dlsym(handle, ifn);
    if (driver == NULL) {
        dlclose(handle);
    }
    return driver;
}

static bool init_driver_list() {
    // If driver is not empty, means initialization has been done.
    // But it prevent re-init driver when lpac running.
    if (!list_empty(&drivers)) {
        return true;
    }

    _cleanup_free_ char *LPAC_DRIVER_HOME = get_driver_path();
    if (LPAC_DRIVER_HOME == NULL)
        return false;
    _cleanup_dir_ DIR *driver_dir = opendir(LPAC_DRIVER_HOME);
    if (driver_dir == NULL) {
        return true;
    }

    struct dirent *entry;
    while ((entry = readdir(driver_dir)) != NULL) {
        if (!strncmp(entry->d_name, "driver_", 7) && ends_with(entry->d_name, dynlib_suffix)) {
            const struct euicc_driver *driver = find_driver_by_path(LPAC_DRIVER_HOME, entry->d_name);
            struct euicc_drivers_list *tmp = (struct euicc_drivers_list *)calloc(1, sizeof(struct euicc_drivers_list));
            if (tmp == NULL) {
                return false;
            }
            tmp->driver = driver;
            list_add_tail(&tmp->list, &drivers);
        }
    }

    for (size_t i = 0; builtin_drivers[i] != NULL; i++) {
        struct euicc_drivers_list *tmp = (struct euicc_drivers_list *)calloc(1, sizeof(struct euicc_drivers_list));
        if (tmp == NULL) {
            return false;
        }
        tmp->driver = builtin_drivers[i];
        list_add_tail(&tmp->list, &drivers);
    }
    return true;
}

static const struct euicc_driver *find_driver_by_name(const enum euicc_driver_type type, const char *name) {
    char *driver_type = NULL;
    if (type == DRIVER_APDU) {
        driver_type = "apdu";
    } else if (type == DRIVER_HTTP) {
        driver_type = "http";
    } else {
        return NULL;
    }

    _cleanup_free_ char *LPAC_DRIVER_HOME = get_driver_path();
    if (LPAC_DRIVER_HOME == NULL)
        return false;

    size_t driver_name_len = 7 + strlen(driver_type) + 1 + strlen(name) + strlen(dynlib_suffix) + 1;
    _cleanup_free_ char *driver_name = calloc(driver_name_len, sizeof(char));
    snprintf(driver_name, driver_name_len, "driver_%s_%s%s", driver_type, name, dynlib_suffix);

    const struct euicc_driver *driver = find_driver_by_path(LPAC_DRIVER_HOME, driver_name);
    // Lookup built-in drivers if not found in dynamic drivers
    if (driver == NULL) {
        for (size_t i = 0; builtin_drivers[i] != NULL; i++) {
            const struct euicc_driver *j = builtin_drivers[i];
            if (j->type == type && !strcmp(j->name, name)) {
                driver = j;
                break;
            }
        }
    }
    return driver;
}

// If backend is not specified, find the certain driver in builtin order.
// The order is copy from old code, so the behavior will not change.
// TODO: Implement user-customized and packager-customized order.
static const struct euicc_driver *find_driver_fallback(const enum euicc_driver_type type) {
    // clang-format off
    static const char *http_fallback_order[] = {
        "winhttp",
        "curl",
        NULL
    };
    static const char *apdu_fallback_order[] = {
        "gbinder_hidl",
        "mbim",
        "qmi",
        "qmi_qrtr",
        "uqmi",
        "pcsc",
        "at",
        NULL
    };
    // clang-format on
    const char **fallback_order = {NULL};
    if (type == DRIVER_APDU) {
        fallback_order = apdu_fallback_order;
    } else if (type == DRIVER_HTTP) {
        fallback_order = http_fallback_order;
    } else {
        return NULL;
    }
    // Try to find driver in fallback list.
    for (int i = 0; fallback_order[i] != NULL; i++) {
        const struct euicc_driver *driver = find_driver_by_name(type, fallback_order[i]);
        if (driver != NULL) {
            return driver;
        }
    }
    // Load all driver and try to find available one.
    // stdio are always builtin so it should fallback to it as last.
    if (init_driver_list()) {
        list_for_each_entry_scoped(it, struct euicc_drivers_list, &drivers, list) {
            const struct euicc_driver *driver = it->driver;
            if (driver->type == type) {
                return driver;
            }
        }
    }
    return NULL;
}

static inline const struct euicc_driver *find_driver(const enum euicc_driver_type type, const char *name) {
    if (name == NULL) {
        return find_driver_fallback(type);
    } else {
        return find_driver_by_name(type, name);
    }
}

int euicc_driver_list(int argc, char **argv) {
    if (!init_driver_list()) {
        fputs("Driver list initialization failed.\n", stderr);
        return -1;
    }
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
        return -1;

    const struct euicc_driver *driver = NULL;
    cJSON *driver_name = NULL;
    cJSON *apdu_drivers = cJSON_CreateArray();
    cJSON *http_drivers = cJSON_CreateArray();
    if (apdu_drivers == NULL)
        return -1;
    list_for_each_entry_scoped(it, struct euicc_drivers_list, &drivers, list) {
        const struct euicc_driver *driver = it->driver;
        driver_name = cJSON_CreateString(driver->name);
        if (driver_name == NULL)
            return -1;
        if (driver->type == DRIVER_APDU) {
            cJSON_AddItemToArray(apdu_drivers, driver_name);
        } else if (driver->type == DRIVER_HTTP) {
            cJSON_AddItemToArray(http_drivers, driver_name);
        }
    }
    cJSON_AddItemToObject(payload, ENV_APDU_DRIVER, apdu_drivers);
    cJSON_AddItemToObject(payload, ENV_HTTP_DRIVER, http_drivers);

    json_print("driver", payload);
    return 0;
}

int euicc_driver_init(const char *apdu_driver_name, const char *http_driver_name) {
    _driver_apdu = find_driver(DRIVER_APDU, apdu_driver_name);
    if (_driver_apdu == NULL) {
        fprintf(stderr, "No APDU driver found\n");
        return -1;
    }

    _driver_http = find_driver(DRIVER_HTTP, http_driver_name);
    if (_driver_http == NULL) {
        fprintf(stderr, "No HTTP driver found\n");
        return -1;
    }

    if (_driver_apdu->init(&euicc_driver_interface_apdu)) {
        fprintf(stderr, "APDU driver init failed\n");
        return -1;
    }

    if (_driver_http->init(&euicc_driver_interface_http)) {
        fprintf(stderr, "HTTP driver init failed\n");
        return -1;
    }

    return 0;
}

void euicc_driver_fini() {
    if (_driver_apdu != NULL && _driver_apdu->fini != NULL) {
        _driver_apdu->fini(&euicc_driver_interface_apdu);
    }
    if (_driver_http != NULL && _driver_http->fini != NULL) {
        _driver_http->fini(&euicc_driver_interface_http);
    }
}

int euicc_driver_main_apdu(const int argc, char **argv) {
    if (_driver_apdu == NULL) {
        fprintf(stderr, "No APDU driver found\n");
        return -1;
    }
    if (_driver_apdu->main == NULL) {
        fprintf(stderr, "The APDU driver '%s' does not support main function\n", _driver_apdu->name);
        return -1;
    }
    return _driver_apdu->main(&euicc_driver_interface_apdu, argc, argv);
}

int euicc_driver_main_http(const int argc, char **argv) {
    if (_driver_http == NULL) {
        fprintf(stderr, "No HTTP driver found\n");
        return -1;
    }
    if (_driver_http->main == NULL) {
        fprintf(stderr, "The HTTP driver '%s' does not support main function\n", _driver_http->name);
        return -1;
    }
    return _driver_http->main(&euicc_driver_interface_http, argc, argv);
}
