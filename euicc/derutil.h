#include <inttypes.h>

struct derutils_node
{
    uint16_t tag;
    uint32_t length;
    const uint8_t *value;
    struct
    {
        const uint8_t *ptr;
        uint32_t length;
    } self;
    struct
    {
        uint32_t relative_offset;
        struct derutils_node *child;
        struct derutils_node *next;
    } pack;
};

int euicc_derutil_unpack_first(struct derutils_node *result, const uint8_t *buffer, uint32_t buffer_len);
int euicc_derutil_unpack_next(struct derutils_node *result, struct derutils_node *prev, const uint8_t *buffer, uint32_t buffer_len);
int euicc_derutil_unpack_find_alias_tags(struct derutils_node *result, const uint16_t *tags, uint32_t tags_count, const uint8_t *buffer, uint32_t buffer_len);
int euicc_derutil_unpack_find_tag(struct derutils_node *result, uint16_t tag, const uint8_t *buffer, uint32_t buffer_len);

int euicc_derutil_pack(uint8_t *buffer, uint32_t *buffer_len, struct derutils_node *node);
int euicc_derutil_pack_alloc(uint8_t **buffer, uint32_t *buffer_len, struct derutils_node *node);

long euicc_derutil_convert_bin2long(const uint8_t *buffer, uint32_t buffer_len);
int euicc_derutil_convert_long2bin(uint8_t *buffer, uint32_t *buffer_len, long value);
int euicc_derutil_convert_bits2bin(uint8_t *buffer, uint32_t buffer_len, const uint32_t *bits, uint32_t bits_count);
int euicc_derutil_convert_bits2bin_alloc(uint8_t **buffer, uint32_t *buffer_len, const uint32_t *bits, uint32_t bits_count);
int euicc_derutil_convert_bin2bits_str(const char ***output, const uint8_t *buffer, int buffer_len, const char **desc);
