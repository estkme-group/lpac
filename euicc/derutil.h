#pragma once

#include <inttypes.h>

uint8_t *euicc_derutil_tag_leftpad(uint8_t *buffer, unsigned buffer_length, unsigned data_offset, uint16_t tag);
int euicc_derutil_tag_find(uint8_t **rptr, uint8_t *buffer, uint32_t buffer_len, uint16_t *target_tag, uint8_t retfirst);
