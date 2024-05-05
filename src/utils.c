#include "string.h"

int is_valid_fqdn_name(const char *name)
{
    int count = 0;
    int allowed;
    for (unsigned long i = strlen(name); i > 0; i--) {
        if (name[i] == '.')
        {
            count = 0;
            continue;
        }
        allowed = (name[i] >= '0' && name[i] <= '9') ||
                  (name[i] >= 'a' && name[i] <= 'z') ||
                  (name[i] >= 'A' && name[i] <= 'Z');
        if (!allowed || count > 63)
        {
            return 0;
        }
        count++;
    }
    if (count > 63)
    {
        return 0;
    }
    return 1;
}
