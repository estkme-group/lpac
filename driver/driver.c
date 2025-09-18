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
#    include <mach-o/dyld.h>
#    include <sys/syslimits.h>
#elif defined(_WIN32)
#    include <errhandlingapi.h>
#    include <libloaderapi.h>
#    include <winerror.h>
#endif

#ifdef LPAC_WITH_DRIVER_APDU_GBINDER
#    include "driver/apdu/gbinder_hidl.h"
#endif

#ifdef LPAC_WITH_DRIVER_APDU_MBIM
#    include "driver/apdu/mbim.h"
#endif

#ifdef LPAC_WITH_DRIVER_APDU_QMI
#    include "driver/apdu/qmi.h"
#endif

#ifdef LPAC_WITH_DRIVER_APDU_QMI_QRTR
#    include "driver/apdu/qmi_qrtr.h"
#endif

#ifdef LPAC_WITH_DRIVER_APDU_PCSC
#    include "driver/apdu/pcsc.h"
#endif
#ifdef LPAC_WITH_DRIVER_APDU_AT
#    include "driver/apdu/at.h"
#endif
#ifdef LPAC_WITH_DRIVER_HTTP_CURL
#    include "driver/http/curl.h"
#endif
#ifdef LPAC_WITH_DRIVER_HTTP_WINHTTP
#    include "driver/http/winhttp.h"
#endif
#include "stdio/stdio.h"

static const struct euicc_driver *drivers[] = {
#ifdef LPAC_WITH_DRIVER_APDU_GBINDER
    &driver_apdu_gbinder_hidl,
#endif
#ifdef LPAC_WITH_DRIVER_APDU_MBIM
    &driver_apdu_mbim,
#endif
#ifdef LPAC_WITH_DRIVER_APDU_QMI
    &driver_apdu_qmi,
#endif
#ifdef LPAC_WITH_DRIVER_APDU_QMI_QRTR
    &driver_apdu_qmi_qrtr,
#endif
#ifdef LPAC_WITH_DRIVER_APDU_PCSC
    &driver_apdu_pcsc,
#endif
#ifdef LPAC_WITH_DRIVER_APDU_AT
    &driver_apdu_at,
#endif
#ifdef LPAC_WITH_DRIVER_HTTP_WINHTTP // Prefer to use WINHTTP
    &driver_http_winhttp,
#endif
#ifdef LPAC_WITH_DRIVER_HTTP_CURL
    &driver_http_curl,
#endif
    &driver_apdu_stdio,        &driver_http_stdio, NULL,
};

static const struct euicc_driver *_driver_apdu = NULL;
static const struct euicc_driver *_driver_http = NULL;

struct euicc_apdu_interface euicc_driver_interface_apdu;
struct euicc_http_interface euicc_driver_interface_http;

struct euicc_drivers_list {
    struct euicc_driver *driver;
    struct list_head list;
};

static LIST_HEAD(dynamic_drivers);

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

#if defined(__linux__)
static const char *get_runpath() {
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
            strtab = (const char *)dyn->d_un.d_val;
        }
    }
    if (runpath) {
        return strtab + runpath->d_un.d_val;
    } else {
        return NULL;
    }
}

#else
// FIXME: I still didn't find the way to determine RUNPATH on non-Linux.
//        So use $ORIGIN as a temporary solution
static char *get_runpath() { return get_origin(); }
#endif

static char *get_first_runpath(const char *runpath) {
    if (runpath == NULL)
        return NULL;
    _cleanup_free_ char *runpath1 = strdup(runpath);
    if (runpath1 == NULL)
        return NULL;
    // runpath is not empty, so strtok shouldn't return NULL when first call.
    return strdup(strtok(runpath1, ":"));
}

#if defined(__unix__) || defined(__unix)
static bool init_dynamic_driver_list() {
    _cleanup_free_ char *LPAC_DRIVER_HOME = get_first_runpath(get_runpath());
    if (LPAC_DRIVER_HOME == NULL)
        return false;
    if (!strcmp(LPAC_DRIVER_HOME, "$ORIGIN")) {
        free(LPAC_DRIVER_HOME);
        LPAC_DRIVER_HOME = get_origin();
        if (LPAC_DRIVER_HOME == NULL)
            return false;
    }
    char *tmp = realloc(LPAC_DRIVER_HOME, strlen(LPAC_DRIVER_HOME) + 8 + 1);
    if (tmp == NULL)
        return false;
    LPAC_DRIVER_HOME = tmp;
    strcat(LPAC_DRIVER_HOME, "/drivers");

    _cleanup_dir_ DIR *driver_dir = opendir(LPAC_DRIVER_HOME);
    if (driver_dir == NULL) {
        return true;
    }
#    if defined(_WIN32)
    const char *suffix = ".dll";
#    elif defined(__APPLE__) && defined(__MACH__)
    const char *suffix = ".dylib";
#    elif defined(__unix__) || defined(__unix)
    const char *suffix = ".so";
#    else
#        error "Unsupported platform."
#    endif

    struct dirent *entry;
    while ((entry = readdir(driver_dir)) != NULL) {
        if ((entry->d_type & (DT_LNK | DT_REG | DT_UNKNOWN)) && ends_with(entry->d_name, suffix)) {
            size_t fullpath_len = strlen(LPAC_DRIVER_HOME) + strlen(entry->d_name) + 2;
            _cleanup_free_ char *fullpath = calloc(fullpath_len, sizeof(char));
            snprintf(fullpath, fullpath_len, "%s/%s", LPAC_DRIVER_HOME, entry->d_name);
            void *handle = dlopen(fullpath, RTLD_NOW);
            if (handle == NULL) {
                continue;
            }
            _cleanup_free_ char *ifn = remove_suffix(entry->d_name, suffix);
            if (ifn == NULL) {
                continue;
            }
            struct euicc_driver *driver = dlsym(handle, ifn);
            if (driver == NULL) {
                dlclose(handle);
                continue;
            }
            struct euicc_drivers_list *tmp = (struct euicc_drivers_list *)calloc(1, sizeof(struct euicc_drivers_list));
            if (tmp == NULL) {
                return false;
            }
            tmp->driver = driver;
            list_add_tail(&tmp->list, &dynamic_drivers);
        }
    }
    return true;
}
#else
static bool init_dynamic_driver_list() { return true; }
#endif

static const struct euicc_driver *find_driver(const enum euicc_driver_type type, const char *name) {
    struct euicc_drivers_list *it;
    list_for_each_entry(it, &dynamic_drivers, list) {
        const struct euicc_driver *driver = it->driver;
        if (driver->type != type)
            continue;
        if (name == NULL || strcasecmp(driver->name, name) == 0)
            return driver;
    }
    for (int i = 0; drivers[i] != NULL; i++) {
        const struct euicc_driver *driver = drivers[i];
        if (driver->type != type)
            continue;
        if (name == NULL || strcasecmp(driver->name, name) == 0)
            return driver;
    }
    return NULL;
}

int euicc_driver_list(int argc, char **argv) {
    cJSON *payload = cJSON_CreateObject();
    if (payload == NULL)
        return -1;

    const struct euicc_driver *driver = NULL;
    cJSON *driver_name = NULL;
    cJSON *apdu_drivers = cJSON_CreateArray();
    cJSON *http_drivers = cJSON_CreateArray();
    if (apdu_drivers == NULL)
        return -1;
    for (int i = 0; drivers[i] != NULL; i++) {
        driver = drivers[i];
        driver_name = cJSON_CreateString(driver->name);
        if (driver_name == NULL)
            return -1;
        if (driver->type == DRIVER_APDU) {
            cJSON_AddItemToArray(apdu_drivers, driver_name);
        } else if (driver->type == DRIVER_HTTP) {
            cJSON_AddItemToArray(http_drivers, driver_name);
        }
    }
    struct euicc_drivers_list *it;
    list_for_each_entry(it, &dynamic_drivers, list) {
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
    init_dynamic_driver_list();
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
