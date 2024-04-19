#include "derutil.h"

#include <stdlib.h>
#include <string.h>

int euicc_derutil_unpack_first(struct euicc_derutil_node *result, const uint8_t *buffer, uint32_t buffer_len)
{
    const uint8_t *cptr;
    uint32_t rlen;

    cptr = buffer;
    rlen = buffer_len;

    memset(result, 0x00, sizeof(struct euicc_derutil_node));

    if (rlen < 1)
    {
        return -1;
    }

    result->tag = *cptr;
    cptr++;
    rlen--;

    if ((result->tag & 0x1F) == 0x1F)
    {
        if (rlen < 1)
        {
            return -1;
        }
        result->tag = (result->tag << 8) | *cptr;
        cptr++;
        rlen--;
    }

    if (rlen < 1)
    {
        return -1;
    }

    result->length = *cptr;
    cptr++;
    rlen--;

    if (result->length & 0x80)
    {
        uint8_t lengthlen = result->length & 0x7F;
        if (rlen < lengthlen)
        {
            return -1;
        }

        result->length = 0;
        for (int i = 0; i < lengthlen; i++)
        {
            result->length = (result->length << 8) | *cptr;
            cptr++;
            rlen--;
        }
    }

    if (rlen < result->length)
    {
        return -1;
    }

    result->value = cptr;

    result->self.ptr = buffer;
    result->self.length = result->value - result->self.ptr + result->length;

    return 0;
}

int euicc_derutil_unpack_next(struct euicc_derutil_node *result, struct euicc_derutil_node *prev, const uint8_t *buffer, uint32_t buffer_len)
{
    const uint8_t *cptr;
    uint32_t rlen;

    cptr = prev->self.ptr + prev->self.length;
    rlen = buffer_len - (cptr - buffer);

    return euicc_derutil_unpack_first(result, cptr, rlen);
}

int euicc_derutil_unpack_find_alias_tags(struct euicc_derutil_node *result, const uint16_t *tags, uint32_t tags_count, const uint8_t *buffer, uint32_t buffer_len)
{
    result->self.ptr = buffer;
    result->self.length = 0;

    while (euicc_derutil_unpack_next(result, result, buffer, buffer_len) == 0)
    {
        for (uint32_t i = 0; i < tags_count; i++)
        {
            if (result->tag == tags[i])
            {
                return 0;
            }
        }
    }

    return -1;
}

int euicc_derutil_unpack_find_tag(struct euicc_derutil_node *result, uint16_t tag, const uint8_t *buffer, uint32_t buffer_len)
{
    return euicc_derutil_unpack_find_alias_tags(result, &tag, 1, buffer, buffer_len);
}

static void euicc_derutil_pack_sizeof_single_node(struct euicc_derutil_node *node)
{
    node->self.length = 0;

    if (node->pack.headless)
    {
        node->self.length = node->length;
        return;
    }

    if (node->tag >> 8)
    {
        node->self.length += 2;
    }
    else
    {
        node->self.length += 1;
    }

    if (node->length < 0x80)
    {
        node->self.length += 1;
    }
    else
    {
        uint8_t lengthlen = 0;
        uint32_t length = node->length;
        while (length)
        {
            length >>= 8;
            lengthlen++;
        }
        node->self.length += 1 + lengthlen;
    }

    node->self.length += node->length;
}

static int euicc_derutil_pack_iterate_size_and_relative_offset(struct euicc_derutil_node *node, struct euicc_derutil_node *parent, uint32_t relative_offset)
{
    uint32_t full_size = 0;

    while (node)
    {
        node->pack.relative_offset = relative_offset;

        if (node->pack.child)
        {
            node->length = 0;
            euicc_derutil_pack_iterate_size_and_relative_offset(node->pack.child, node, relative_offset);
        }

        euicc_derutil_pack_sizeof_single_node(node);

        if (parent)
        {
            parent->length += node->self.length;
        }

        relative_offset += node->self.length;
        full_size += node->self.length;
        node = node->pack.next;
    }

    return full_size;
}

static void euicc_derutil_pack_iterate_ptrs(struct euicc_derutil_node *node, uint8_t *wptr)
{
    while (node)
    {
        node->self.ptr = wptr;

        if (node->pack.child)
        {
            euicc_derutil_pack_iterate_ptrs(node->pack.child, (wptr + node->self.length - node->length));
        }

        wptr += node->self.length;
        node = node->pack.next;
    }
}

static void euicc_derutil_pack_copydata_single_node(struct euicc_derutil_node *node)
{
    uint8_t *buffer = (uint8_t *)(node->self.ptr);

    if (node->pack.headless)
    {
        memcpy(buffer, node->value, node->length);
        return;
    }

    if (node->tag >> 8)
    {
        *buffer = node->tag >> 8;
        buffer++;
    }
    *buffer = node->tag & 0xFF;
    buffer++;

    if (node->length < 0x80)
    {
        *buffer = node->length;
        buffer++;
    }
    else
    {
        uint8_t lengthlen = 0;
        uint32_t length = node->length;
        while (length)
        {
            length >>= 8;
            lengthlen++;
        }
        *buffer = 0x80 | lengthlen;
        buffer++;
        for (int i = lengthlen - 1; i >= 0; i--)
        {
            *buffer = (node->length >> (i * 8)) & 0xFF;
            buffer++;
        }
    }

    if (node->value && !node->pack.child)
    {
        memcpy(buffer, node->value, node->length);
    }
    else
    {
        node->value = buffer;
    }
}

static void euicc_derutil_pack_iterate_copydata(struct euicc_derutil_node *node)
{
    while (node)
    {
        euicc_derutil_pack_copydata_single_node(node);

        if (node->pack.child)
        {
            euicc_derutil_pack_iterate_copydata(node->pack.child);
        }

        node = node->pack.next;
    }
}

static void euicc_derutil_pack_finish(struct euicc_derutil_node *node, uint8_t *buffer)
{
    euicc_derutil_pack_iterate_ptrs(node, buffer);
    euicc_derutil_pack_iterate_copydata(node);
}

int euicc_derutil_pack(uint8_t *buffer, uint32_t *buffer_len, struct euicc_derutil_node *node)
{
    uint32_t required_size = 0;
    required_size = euicc_derutil_pack_iterate_size_and_relative_offset(node, NULL, 0);
    if (*buffer_len < required_size)
    {
        return -1;
    }
    euicc_derutil_pack_finish(node, buffer);
    *buffer_len = required_size;
    return 0;
}

int euicc_derutil_pack_alloc(uint8_t **buffer, uint32_t *buffer_len, struct euicc_derutil_node *node)
{
    uint32_t required_size = 0;
    required_size = euicc_derutil_pack_iterate_size_and_relative_offset(node, NULL, 0);
    *buffer_len = required_size;
    *buffer = malloc(*buffer_len);
    if (!*buffer)
    {
        return -1;
    }
    euicc_derutil_pack_finish(node, *buffer);
    return 0;
}

long euicc_derutil_convert_bin2long(const uint8_t *buffer, uint32_t buffer_len)
{
    long result = 0;
    for (uint32_t i = 0; i < buffer_len; i++)
    {
        result = (result << 8) | buffer[i];
    }
    return result;
}

int euicc_derutil_convert_long2bin(uint8_t *buffer, uint32_t *buffer_len, long value)
{
    uint8_t required_len = 1;

    for (int i = 1; i < sizeof(value); i++)
    {
        if ((value >> (i * 8)))
        {
            required_len++;
        }
        else
        {
            if (value > 0)
            {
                if ((value >> ((i - 1) * 8)) & 0x80)
                {
                    required_len++;
                }
            }
            break;
        }
    }

    if (required_len > *buffer_len)
    {
        return -1;
    }

    for (int i = 0; i < required_len; i++)
    {
        buffer[i] = (value >> ((required_len - i - 1) * 8)) & 0xFF;
    }

    *buffer_len = required_len;

    return 0;
}

static uint32_t euicc_derutil_convert_bits2bin_sizeof(const uint32_t *bits, uint32_t bits_count)
{
    uint32_t max_bit = 0;
    for (uint32_t i = 0; i < bits_count; i++)
    {
        if (bits[i] > max_bit)
        {
            max_bit = bits[i];
        }
    }

    return ((max_bit + 8) / 8) + 1;
}

int euicc_derutil_convert_bits2bin(uint8_t *buffer, uint32_t buffer_len, const uint32_t *bits, uint32_t bits_count)
{
    if (buffer_len < euicc_derutil_convert_bits2bin_sizeof(bits, bits_count))
    {
        return -1;
    }

    memset(buffer, 0x00, buffer_len);

    buffer[0] = 0x00;

    for (uint32_t i = 0; i < bits_count; i++)
    {
        buffer[(bits[i] / 8) + 1] |= 1 << (7 - (bits[i] % 8));
    }

    return 0;
}

int euicc_derutil_convert_bits2bin_alloc(uint8_t **buffer, uint32_t *buffer_len, const uint32_t *bits, uint32_t bits_count)
{
    *buffer_len = euicc_derutil_convert_bits2bin_sizeof(bits, bits_count);
    *buffer = malloc(*buffer_len);
    if (!*buffer)
    {
        return -1;
    }
    return euicc_derutil_convert_bits2bin(*buffer, *buffer_len, bits, bits_count);
}

int euicc_derutil_convert_bin2bits_str(const char ***output, const uint8_t *buffer, int buffer_len, const char **desc)
{
    int max_cap_len = 0;
    int flags_reg;
    int flags_count = 0;
    const char **wptr;
    char unused;

    *output = NULL;

    if (buffer_len < 1)
    {
        return -1;
    }

    unused = *buffer;

    buffer++;
    buffer_len--;

    for (max_cap_len = 0; desc[max_cap_len]; max_cap_len++)
        ;

    for (int j = 0; j < buffer_len; j++)
    {
        if (j == buffer_len - 1)
        {
            flags_reg = buffer[j] & ~(0xFF >> (8 - unused));
        }
        else
        {
            flags_reg = buffer[j];
        }
        for (int i = 0; (i < 8) && ((j * 8 + i) < max_cap_len); i++)
        {
            if (flags_reg & 0x80)
            {
                flags_count++;
            }
            flags_reg <<= 1;
        }
    }

    wptr = calloc(flags_count + 1, sizeof(char *));
    if (!wptr)
    {
        return -1;
    }
    *output = wptr;

    for (int j = 0; j < buffer_len; j++)
    {
        if (j == buffer_len - 1)
        {
            flags_reg = buffer[j] & ~(0xFF >> (8 - unused));
        }
        else
        {
            flags_reg = buffer[j];
        }

        for (int i = 0; (i < 8) && ((j * 8 + i) < max_cap_len); i++)
        {
            if (flags_reg & 0x80)
            {
                *(wptr++) = desc[j * 8 + i];
            }
            flags_reg <<= 1;
        }
    }

    return 0;
}
