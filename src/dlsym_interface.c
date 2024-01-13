#include "dlsym_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef __MINGW32__
#include <dlfcn-win32/dlfcn.h>
#else
#include <dlfcn.h>
#endif

#if defined (_WIN32) || defined (__CYGWIN__)
#define INTERFACELIB_EXTENSION "dll"
#elif defined(__APPLE__)
#define INTERFACELIB_EXTENSION "dylib"
#else
#define INTERFACELIB_EXTENSION "so"
#endif

static struct applet_entry applet_apdu = {
    .name = "apdu",
    .main = NULL,
};

static struct applet_entry applet_http = {
    .name = "http",
    .main = NULL,
};

static const struct applet_entry *applets[] = {
    &applet_apdu,
    &applet_http,
    NULL,
};

static const char *libapduinterface_path = NULL;
static void *apdu_interface_dlhandle = NULL;
struct euicc_apdu_interface dlsym_apdu_interface = {0};
static int (*libapduinterface_init)(struct euicc_apdu_interface *ifstruct) = NULL;
static int (*libapduinterface_main)(int argc, char **argv) = NULL;

static const char *libhttpinterface_path = NULL;
static void *http_interface_dlhandle = NULL;
struct euicc_http_interface dlsym_http_interface = {0};
static int (*libhttpinterface_init)(struct euicc_http_interface *ifstruct) = NULL;
static int (*libhttpinterface_main)(int argc, char **argv) = NULL;

static void dlsym_interfaces_get_path(void)
{
    if (!(libapduinterface_path = getenv("APDU_INTERFACE")))
    {
        libapduinterface_path = "libapduinterface_pcsc." INTERFACELIB_EXTENSION;
    }

    if (!(libhttpinterface_path = getenv("HTTP_INTERFACE")))
    {
        libhttpinterface_path = "libhttpinterface_curl." INTERFACELIB_EXTENSION;
    }
}

static int dlsym_interface_get_dlhandle(void)
{
    if (!(apdu_interface_dlhandle = dlopen(libapduinterface_path, RTLD_LAZY)))
    {
        apdu_interface_dlhandle = NULL;
        fprintf(stderr, "APDU interface env missing, current: APDU_INTERFACE=%s err:%s\n", libapduinterface_path, dlerror());
        return -1;
    }

    if (!(http_interface_dlhandle = dlopen(libhttpinterface_path, RTLD_LAZY)))
    {
        http_interface_dlhandle = NULL;
        fprintf(stderr, "HTTP interface env missing, current: HTTP_INTERFACE=%s err:%s\n", libhttpinterface_path, dlerror());
    }

    return 0;
}

int dlsym_interface_init()
{
    dlsym_interfaces_get_path();

    if (dlsym_interface_get_dlhandle())
    {
        return -1;
    }

    if (apdu_interface_dlhandle)
    {
        libapduinterface_init = dlsym(apdu_interface_dlhandle, "libapduinterface_init");
        if (!libapduinterface_init)
        {
            fprintf(stderr, "APDU library broken: missing libapduinterface_init\n");
            return -1;
        }
        if (libapduinterface_init(&dlsym_apdu_interface) < 0)
        {
            fprintf(stderr, "APDU library init error\n");
            return -1;
        }
        libapduinterface_main = dlsym(apdu_interface_dlhandle, "libapduinterface_main");
        if (!libapduinterface_main)
        {
            fprintf(stderr, "APDU library broken: missing libapduinterface_main\n");
            return -1;
        }
        applet_apdu.main = libapduinterface_main;
    }

    if (http_interface_dlhandle)
    {
        libhttpinterface_init = dlsym(http_interface_dlhandle, "libhttpinterface_init");
        if (!libhttpinterface_init)
        {
            fprintf(stderr, "HTTP library broken: missing libhttpinterface_init\n");
            return -1;
        }
        if (libhttpinterface_init(&dlsym_http_interface) < 0)
        {
            fprintf(stderr, "HTTP library init error\n");
            return -1;
        }
        libhttpinterface_main = dlsym(http_interface_dlhandle, "libhttpinterface_main");
        if (!libhttpinterface_main)
        {
            fprintf(stderr, "HTTP library broken: missing libhttpinterface_main\n");
            return -1;
        }
        applet_http.main = libhttpinterface_main;
    }

    return 0;
}

static int dlsym_interface_applet_main(int argc, char **argv)
{
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_dlsym_interface = {
    .name = "driver",
    .main = dlsym_interface_applet_main,
};
