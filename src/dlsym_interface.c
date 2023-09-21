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

static void *apdu_interface_dlhandle;
struct euicc_apdu_interface dlsym_apdu_interface = {0};
static void *es9p_interface_dlhandle;
struct euicc_es9p_interface dlsym_es9p_interface = {0};

int dlsym_interface_init()
{
    const char *apduinterface_so;
    const char *es9pinterface_so;

    if (!(apduinterface_so = getenv("APDU_INTERFACE")))
    {
        apduinterface_so = "./libapduinterface." POSTFIX;
    }

    if (!(es9pinterface_so = getenv("ES9P_INTERFACE")))
    {
        es9pinterface_so = "./libes9pinterface." POSTFIX;
    }

    if (!(apdu_interface_dlhandle = dlopen(apduinterface_so, RTLD_LAZY)))
    {
        if (!(apdu_interface_dlhandle = dlopen("libapduinterface." POSTFIX, RTLD_LAZY)))
        {
            fprintf(stderr, "APDU interface missing, system can't continue.\nProvide it as \"libapduinterface." POSTFIX "\" or set \"APDU_INTERFACE=\" env variable\nCurrent: APDU_INTERFACE=%s\n", getenv("APDU_INTERFACE"));
            return -1;
        }
    }
    if (!(dlsym_apdu_interface.connect = dlsym(apdu_interface_dlhandle, "euicc_apdu_interface_connect")))
    {
        fprintf(stderr, "apduinterface: missing symbol 'euicc_apdu_interface_connect'\n");
        return -1;
    }
    if (!(dlsym_apdu_interface.disconnect = dlsym(apdu_interface_dlhandle, "euicc_apdu_interface_disconnect")))
    {
        fprintf(stderr, "apduinterface: missing symbol 'euicc_apdu_interface_disconnect'\n");
        return -1;
    }
    if (!(dlsym_apdu_interface.logic_channel_open = dlsym(apdu_interface_dlhandle, "euicc_apdu_interface_logic_channel_open")))
    {
        fprintf(stderr, "apduinterface: missing symbol 'euicc_apdu_interface_logic_channel_open'\n");
        return -1;
    }
    if (!(dlsym_apdu_interface.logic_channel_close = dlsym(apdu_interface_dlhandle, "euicc_apdu_interface_logic_channel_close")))
    {
        fprintf(stderr, "apduinterface: missing symbol 'euicc_apdu_interface_logic_channel_close'\n");
        return -1;
    }
    if (!(dlsym_apdu_interface.transmit = dlsym(apdu_interface_dlhandle, "euicc_apdu_interface_transmit")))
    {
        fprintf(stderr, "apduinterface: missing symbol 'euicc_apdu_interface_transmit'\n");
        return -1;
    }

    if (!(es9p_interface_dlhandle = dlopen(es9pinterface_so, RTLD_LAZY)))
    {
        if (!(es9p_interface_dlhandle = dlopen("libes9pinterface." POSTFIX, RTLD_LAZY)))
        {
            fprintf(stderr, "ES9P interface missing, download & notification process will not work.\nProvide as \"libes9pinterface." POSTFIX "\" or set \"ES9P_INTERFACE=\" env variable\nCurrent: ES9P_INTERFACE=%s\n", getenv("ES9P_INTERFACE"));
            return 0;
        }
    }
    if (!(dlsym_es9p_interface.transmit = dlsym(es9p_interface_dlhandle, "euicc_es9p_interface_transmit")))
    {
        fprintf(stderr, "es9pinterface: missing symbol 'euicc_es9p_interface_transmit'\n");
        return -1;
    }

    return 0;
}
