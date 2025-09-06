// SPDX-License-Identifier: CC0-1.0
// SPDX-License-Copyright: Brad Conte <brad@bradconte.com>
// Original code from https://github.com/B-Con/crypto-algorithms/blob/02b66ec/sha256.h
/*********************************************************************
 * Filename:   sha256.h
 * Author:     Brad Conte (brad AT bradconte.com)
 * Copyright:
 * Disclaimer: This code is presented "as is" without any guarantees.
 * Details:    Defines the API for the corresponding SHA1 implementation.
 *********************************************************************/

#ifndef SHA256_H
#define SHA256_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32 // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE; // 8-bit byte
typedef unsigned int WORD;  // 32-bit word, change to "long" for 16-bit machines

typedef struct {
    BYTE data[64];
    WORD datalen;
    unsigned long long bitlen;
    WORD state[8];
} EUICC_SHA256_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void euicc_sha256_init(EUICC_SHA256_CTX *ctx);
void euicc_sha256_update(EUICC_SHA256_CTX *ctx, const BYTE data[], size_t len);
void euicc_sha256_final(EUICC_SHA256_CTX *ctx, BYTE hash[]);

#endif // SHA256_H
