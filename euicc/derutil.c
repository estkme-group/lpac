#include "derutil.h"
#include <stdio.h>

uint8_t *euicc_derutil_tag_leftpad(uint8_t *buffer, unsigned buffer_length, unsigned data_offset, uint16_t tag)
{
    uint8_t *wp = NULL;
    unsigned data_length;
    uint8_t lengthtag = 0x00;
    uint8_t havelengthtag = 0;
    uint8_t lengthbytes = 0;
    uint8_t totalpadlen = 0;

    data_length = buffer_length - data_offset;

    if (data_length > 65535)
    {
        lengthtag = 0x83;
        havelengthtag = 1;
        lengthbytes = 3;
    }
    else if (data_length > 255)
    {
        lengthtag = 0x82;
        havelengthtag = 1;
        lengthbytes = 2;
    }
    else if (data_length > 127)
    {
        lengthtag = 0x81;
        havelengthtag = 1;
        lengthbytes = 1;
    }
    else
    {
        havelengthtag = 0;
        lengthbytes = 1;
    }

    totalpadlen = 2 + havelengthtag + lengthbytes;
    if (data_offset < totalpadlen)
    {
        return NULL;
    }

    wp = buffer + (data_offset - totalpadlen);
    wp[0] = tag >> 8;
    wp[1] = tag & 0xFF;
    wp += 2;
    if (havelengthtag)
    {
        wp[0] = lengthtag;
        wp += 1;
    }
    // lengthbytes, big endian
    for (int i = 0; i < lengthbytes; i++)
    {
        wp[lengthbytes - i - 1] = (data_length >> (i * 8)) & 0xFF;
    }
    return (buffer + data_offset - totalpadlen);
}

int euicc_derutil_tag_find(uint8_t **rptr, uint8_t *buffer, uint32_t buffer_len, uint16_t *target_tag, uint8_t retfirst)
{
    uint16_t current_tag;
    uint16_t current_length;
    uint8_t state = 0;
    uint8_t *ptr;
    ptr = buffer;
    state = 0;
    while (ptr <= (buffer + buffer_len))
    {
        switch (state)
        {
        case 0:
            current_tag = *ptr;
            if ((current_tag & 0b11111) == 0b11111)
            {
                state = 1;
            }
            else
            {
                state = 2;
            }
            break;
        case 1:
            current_tag = (current_tag << 8) | *ptr;
            state = 2;
            break;
        case 2:
            current_length = *ptr;
            switch (current_length)
            {
            case 0x81:
                state = 3;
                break;
            case 0x82:
                state = 4;
                break;
            default:
                state = 5;
                break;
            }
            break;
        case 3:
            current_length = *ptr;
            state = 5;
            break;
        case 4:
            current_length = *ptr;
            state = 41;
            break;
        case 41:
            current_length = (current_length << 8) | *ptr;
            state = 5;
            break;
        case 5:
            if (retfirst)
            {
                if (target_tag)
                {
                    *target_tag = current_tag;
                }
                if (rptr)
                {
                    *rptr = ptr;
                }
                return current_length;
            }
            else
            {
                if (current_tag == *target_tag)
                {
                    if (rptr)
                    {
                        *rptr = ptr;
                    }
                    return current_length;
                }
                if (current_length == 0)
                {
                    state = 0;
                    continue;
                }
                current_length--;
                if (current_length == 0)
                {
                    state = 0;
                }
            }
            break;
        }
        ptr++;
    }
    return -1;
}
