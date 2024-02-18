#include "derutils.h"

#include <stdlib.h>
#include <string.h>

int derutils_unpack_first(struct derutils_node *result, const uint8_t *buffer, uint32_t buffer_len)
{
    const uint8_t *cptr;
    uint32_t rlen;

    cptr = buffer;
    rlen = buffer_len;

    memset(result, 0x00, sizeof(struct derutils_node));

    if (rlen < 1)
    {
        return -1;
    }

    result->tag = *cptr;
    cptr++;
    rlen--;

    if ((result->tag & 0b11111) == 0b11111)
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

int derutils_unpack_next(struct derutils_node *result, struct derutils_node *prev, const uint8_t *buffer, uint32_t buffer_len)
{
    const uint8_t *cptr;
    uint32_t rlen;

    cptr = prev->self.ptr + prev->self.length;
    rlen = buffer_len - (cptr - buffer);

    return derutils_unpack_first(result, cptr, rlen);
}

int derutils_unpack_find_alias_tags(struct derutils_node *result, const uint16_t *tags, uint32_t tags_count, const uint8_t *buffer, uint32_t buffer_len)
{
    result->self.ptr = buffer;
    result->self.length = 0;

    while (derutils_unpack_next(result, result, buffer, buffer_len) == 0)
    {
        for (int i = 0; i < tags_count; i++)
        {
            if (result->tag == tags[i])
            {
                return 0;
            }
        }
    }

    return -1;
}

int derutils_unpack_find_tag(struct derutils_node *result, uint16_t tag, const uint8_t *buffer, uint32_t buffer_len)
{
    return derutils_unpack_find_alias_tags(result, &tag, 1, buffer, buffer_len);
}

static void derutils_pack_sizeof_single_node(struct derutils_node *node)
{
    node->self.length = 0;

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

static int derutils_pack_iterate_size_and_relative_offset(struct derutils_node *node, struct derutils_node *parent, uint32_t relative_offset)
{
    uint32_t full_size = 0;

    while (node)
    {
        node->pack.relative_offset = relative_offset;

        if (node->pack.child)
        {
            node->length = 0;
            derutils_pack_iterate_size_and_relative_offset(node->pack.child, node, relative_offset);
        }

        derutils_pack_sizeof_single_node(node);

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

static void derutils_pack_iterate_ptrs(struct derutils_node *node, uint8_t *wptr)
{
    while (node)
    {
        node->self.ptr = wptr;

        if (node->pack.child)
        {
            derutils_pack_iterate_ptrs(node->pack.child, (wptr + node->self.length - node->length));
        }

        wptr += node->self.length;
        node = node->pack.next;
    }
}

static void derutils_pack_copydata_single_node(struct derutils_node *node)
{
    uint8_t *buffer = (uint8_t *)(node->self.ptr);

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

static void derutils_pack_iterate_copydata(struct derutils_node *node)
{
    while (node)
    {
        derutils_pack_copydata_single_node(node);

        if (node->pack.child)
        {
            derutils_pack_iterate_copydata(node->pack.child);
        }

        node = node->pack.next;
    }
}

static void derutils_pack_finish(struct derutils_node *node, uint8_t *buffer)
{
    derutils_pack_iterate_ptrs(node, buffer);
    derutils_pack_iterate_copydata(node);
}

int derutils_pack(uint8_t *buffer, uint32_t *buffer_len, struct derutils_node *node)
{
    uint32_t required_size = 0;
    required_size = derutils_pack_iterate_size_and_relative_offset(node, NULL, 0);
    if (*buffer_len < required_size)
    {
        return -1;
    }
    derutils_pack_finish(node, buffer);
    *buffer_len = required_size;
    return 0;
}

int derutils_pack_alloc(uint8_t **buffer, uint32_t *buffer_len, struct derutils_node *node)
{
    uint32_t required_size = 0;
    required_size = derutils_pack_iterate_size_and_relative_offset(node, NULL, 0);
    *buffer_len = required_size;
    *buffer = malloc(*buffer_len);
    if (!*buffer)
    {
        return -1;
    }
    derutils_pack_finish(node, *buffer);
    return 0;
}

long derutils_convert_bin2long(const uint8_t *buffer, uint32_t buffer_len)
{
    long result = 0;
    for (uint32_t i = 0; i < buffer_len; i++)
    {
        result = (result << 8) | buffer[i];
    }
    return result;
}

int derutils_convert_long2bin(uint8_t *buffer, uint32_t *buffer_len, long value)
{
    uint8_t required_len;

    for (int i = 0; i < 8; i++)
    {
        if (value >> (i * 8))
        {
            required_len = i + 1;
        }
    }

    if (required_len > *buffer_len)
    {
        return -1;
    }

    for (int i = required_len - 1; i >= 0; i--)
    {
        buffer[i] = (value >> (i * 8)) & 0xFF;
    }

    *buffer_len = required_len;

    return 0;
}

static uint32_t derutils_convert_bits2bin_sizeof(const uint32_t *bits, uint32_t bits_count)
{
    uint32_t max_bit = 0;
    for (int i = 0; i < bits_count; i++)
    {
        if (bits[i] > max_bit)
        {
            max_bit = bits[i];
        }
    }

    return ((max_bit + 8) / 8) + 1;
}

int derutils_convert_bits2bin(uint8_t *buffer, uint32_t buffer_len, const uint32_t *bits, uint32_t bits_count)
{
    if (buffer_len < derutils_convert_bits2bin_sizeof(bits, bits_count))
    {
        return -1;
    }

    memset(buffer, 0x00, buffer_len);

    buffer[0] = 0x00;

    for (int i = 0; i < bits_count; i++)
    {
        buffer[(bits[i] / 8) + 1] |= 1 << (7 - (bits[i] % 8));
    }

    return 0;
}

int derutils_convert_bits2bin_alloc(uint8_t **buffer, uint32_t *buffer_len, const uint32_t *bits, uint32_t bits_count)
{
    *buffer_len = derutils_convert_bits2bin_sizeof(bits, bits_count);
    *buffer = malloc(*buffer_len);
    if (!*buffer)
    {
        return -1;
    }
    return derutils_convert_bits2bin(*buffer, *buffer_len, bits, bits_count);
}

int derutils_convert_bin2bits_str(const char ***output, const uint8_t *buffer, int buffer_len, const char **desc)
{
    int max_cap_len = 0;
    int flags_reg;
    int flags_count;
    const char **wptr;

    *output = NULL;

    for (max_cap_len = 0; desc[max_cap_len]; max_cap_len++)
        ;

    for (int j = 0; j < buffer_len; j++)
    {
        flags_reg = buffer[j];
        for (int i = 0; (i < 8) && ((j * 8 + i) < max_cap_len); i++)
        {
            if (flags_reg & (0b10000000))
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
        flags_reg = buffer[j];

        for (int i = 0; (i < 8) && ((j * 8 + i) < max_cap_len); i++)
        {
            if (flags_reg & 0b10000000)
            {
                *(wptr++) = desc[j * 8 + i];
            }
            flags_reg <<= 1;
        }
    }

    return 0;
}
