#include "dlsym_interface.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __MINGW32__
#include <dlfcn-win32/dlfcn.h>
#else
#include <dlfcn.h>
#endif

#ifdef __MINGW32__
#define POSTFIX "dll"
#else
#define POSTFIX "so"
#endif

static void *apdu_interface_dlhandle = NULL;
struct euicc_apdu_interface dlsym_apdu_interface = {0};
static void *es9p_interface_dlhandle = NULL;
struct euicc_es9p_interface dlsym_es9p_interface = {0};

int dlsym_interface_init()
{
    const char *libapduinterface_path = NULL;
    const char *libes9pinterface_path = NULL;
    int (*libapduinterface_main)(struct euicc_apdu_interface *ifstruct) = NULL;
    int (*libes9pinterface_main)(struct euicc_es9p_interface *ifstruct) = NULL;

    if (!(libapduinterface_path = getenv("APDU_INTERFACE")))
    {
        libapduinterface_path = "./libapduinterface." POSTFIX;
    }

    if (!(libes9pinterface_path = getenv("ES9P_INTERFACE")))
    {
        libes9pinterface_path = "./libes9pinterface." POSTFIX;
    }

    if (!(apdu_interface_dlhandle = dlopen(libapduinterface_path, RTLD_LAZY)))
    {
        fprintf(stderr, "APDU interface env missing, current: APDU_INTERFACE=%s\n", libapduinterface_path);
        return -1;
    }

    if (!(es9p_interface_dlhandle = dlopen(libes9pinterface_path, RTLD_LAZY)))
    {
        fprintf(stderr, "ES9P interface env missing, current: ES9P_INTERFACE=%s\n", libes9pinterface_path);
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

    if (es9p_interface_dlhandle)
    {
        libes9pinterface_main = dlsym(es9p_interface_dlhandle, "libes9pinterface_main");
        if (!libes9pinterface_main)
        {
            fprintf(stderr, "ES9P library broken\n");
            return -1;
        }
        if (libes9pinterface_main(&dlsym_es9p_interface) < 0)
        {
            fprintf(stderr, "ES9P library init error\n");
            return -1;
        }
    }

    return 0;
}
