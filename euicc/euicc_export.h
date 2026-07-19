#pragma once

#if defined(_WIN32)
#    ifdef LIBEUICC_EXPORTS
#        define EUICC_API __declspec(dllexport)
#    else
#        define EUICC_API __declspec(dllimport)
#    endif
#else
#    define EUICC_API
#endif
