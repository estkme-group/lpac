#include "string.h"
#include "stdbool.h"

bool is_valid_fqdn_name(const char *name)
{
    int count = 0;
    int allowed;
    for (unsigned long i = strlen(name); i > 0; i--)
    {
        if (name[i] == '.')
        {
            if (count == 0)
            {
                return false;
            }
            count = 0;
            continue;
        }
        allowed = (name[i] >= '0' && name[i] <= '9') ||
                  (name[i] >= 'a' && name[i] <= 'z') ||
                  (name[i] >= 'A' && name[i] <= 'Z') ||
                  name[i] == '_' || name[i] == '-';
        if (!allowed || count > 63)
        {
            return false;
        }
        count++;
    }
    if (count > 63 || count == 0)
    {
        return false;
    }
    return true;
}
