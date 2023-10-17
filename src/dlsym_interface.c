#include "dlsym_interface.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __MINGW32__
#include <dlfcn-win32/dlfcn.h>
#else
#include <dlfcn.h>
#endif

#if defined(__MINGW32__)
#define INTERFACELIB_POSTFIX "dll"
#elif defined(__APPLE__)
#define INTERFACELIB_POSTFIX "dylib"
#else
#define INTERFACELIB_POSTFIX "so"
#endif

static void *apdu_interface_dlhandle = NULL;
struct euicc_apdu_interface dlsym_apdu_interface = {0};
static void *http_interface_dlhandle = NULL;
struct euicc_http_interface dlsym_http_interface = {0};

int dlsym_interface_init()
{
    const char *libapduinterface_path = NULL;
    const char *libhttpinterface_path = NULL;
    int (*libapduinterface_main)(struct euicc_apdu_interface *ifstruct) = NULL;
    int (*libhttpinterface_main)(struct euicc_http_interface *ifstruct) = NULL;

    if (!(libapduinterface_path = getenv("APDU_INTERFACE")))
    {
        libapduinterface_path = "./libapduinterface." INTERFACELIB_POSTFIX;
    }

    if (!(libhttpinterface_path = getenv("HTTP_INTERFACE")))
    {
        libhttpinterface_path = "./libhttpinterface." INTERFACELIB_POSTFIX;
    }

    if (!(apdu_interface_dlhandle = dlopen(libapduinterface_path, RTLD_LAZY)))
    {
        fprintf(stderr, "APDU interface env missing, current: APDU_INTERFACE=%s\n", libapduinterface_path);
        return -1;
    }

    if (!(http_interface_dlhandle = dlopen(libhttpinterface_path, RTLD_LAZY)))
    {
        fprintf(stderr, "HTTP interface env missing, current: HTTP_INTERFACE=%s\n", libhttpinterface_path);
    }

    if (apdu_interface_dlhandle)
    {
        libapduinterface_main = dlsym(apdu_interface_dlhandle, "libapduinterface_main");
        if (!libapduinterface_main)
        {
            fprintf(stderr, "APDU library broken\n");
            return -1;
        }
        if (libapduinterface_main(&dlsym_apdu_interface) < 0)
        {
            fprintf(stderr, "APDU library init error\n");
            return -1;
        }
    }

    if (http_interface_dlhandle)
    {
        libhttpinterface_main = dlsym(http_interface_dlhandle, "libhttpinterface_main");
        if (!libhttpinterface_main)
        {
            fprintf(stderr, "HTTP library broken\n");
            return -1;
        }
        if (libhttpinterface_main(&dlsym_http_interface) < 0)
        {
            fprintf(stderr, "HTTP library init error\n");
            return -1;
        }
    }

    return 0;
}
