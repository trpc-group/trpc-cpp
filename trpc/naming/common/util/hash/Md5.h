//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. smhasher
//
// "Derived from the RSA Data Security, Inc. MD5 Message Digest Algorithm"
// Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
// rights reserved.

// License to copy and use this software is granted provided that it
// is identified as the "RSA Data Security, Inc. MD5 Message-Digest
// Algorithm" in all material mentioning or referencing this software
// or this function.

// License is also granted to make and use derivative works provided
// that such works are identified as "derived from the RSA Data
// Security, Inc. MD5 Message-Digest Algorithm" in all material
// mentioning or referencing the derived work.

// RSA Data Security, Inc. makes no representations concerning either
// the merchantability of this software or the suitability of this
// software for any particular purpose. It is provided "as is"
// without express or implied warranty of any kind.

// These notices must be retained in any copies of any part of this
// documentation and/or software.
/*
 * 32-bit integer manipulation macros (little endian)
 */
 
#pragma once

#include <bits/stdint-uintn.h>

#ifndef GET_ULONG_LE
#define GET_ULONG_LE(n, b, i)                                                                                    \
  {                                                                                                              \
    (n) = ((unsigned long)(b)[(i)]) | ((unsigned long)(b)[(i) + 1] << 8) | ((unsigned long)(b)[(i) + 2] << 16) | \
          ((unsigned long)(b)[(i) + 3] << 24);                                                                   \
  }
#endif

#ifndef PUT_ULONG_LE
#define PUT_ULONG_LE(n, b, i)                  \
  {                                            \
    (b)[(i)] = (unsigned char)((n));           \
    (b)[(i) + 1] = (unsigned char)((n) >> 8);  \
    (b)[(i) + 2] = (unsigned char)((n) >> 16); \
    (b)[(i) + 3] = (unsigned char)((n) >> 24); \
  }
#endif
/**
 * \brief          MD5 context structure
 */
typedef struct {
  unsigned long total[2];   /*!< number of bytes processed  */
  unsigned long state[4];   /*!< intermediate digest state  */
  unsigned char buffer[64]; /*!< data block being processed */

  unsigned char ipad[64]; /*!< HMAC: inner padding        */
  unsigned char opad[64]; /*!< HMAC: outer padding        */
} md5_context;

/**
 * \brief          MD5 context setup
 *
 * \param ctx      context to be initialized
 */
void md5_starts(md5_context* ctx);

/**
 * \brief          MD5 process buffer
 *
 * \param ctx      MD5 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
void md5_update(md5_context* ctx, unsigned char* input, int ilen);

/**
 * \brief          MD5 final digest
 *
 * \param ctx      MD5 context
 * \param output   MD5 checksum result
 */
void md5_finish(md5_context* ctx, unsigned char output[16]);

/**
 * \brief          Output = MD5( input buffer )
 *
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   MD5 checksum result
 */
void md5(unsigned char* input, int ilen, unsigned char output[16]);

unsigned int md5hash(const void* input, int len, unsigned int /*seed*/);

void md5_32(const void* key, int len, uint32_t /*seed*/, void* out);
