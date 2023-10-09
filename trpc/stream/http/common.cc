//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/stream/http/common.h"

namespace trpc::stream {

std::string HttpChunkHeader(size_t chunk_length) {
  static constexpr char lut[513] =
      "000102030405060708090a0b0c0d0e0f"
      "101112131415161718191a1b1c1d1e1f"
      "202122232425262728292a2b2c2d2e2f"
      "303132333435363738393a3b3c3d3e3f"
      "404142434445464748494a4b4c4d4e4f"
      "505152535455565758595a5b5c5d5e5f"
      "606162636465666768696a6b6c6d6e6f"
      "707172737475767778797a7b7c7d7e7f"
      "808182838485868788898a8b8c8d8e8f"
      "909192939495969798999a9b9c9d9e9f"
      "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
      "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
      "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
      "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
      "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
      "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";
  size_t idx = sizeof(chunk_length) * 2;
  std::string hex(idx + 2, '0');
  hex[idx] = '\r', hex[idx + 1] = '\n';
  do {
    size_t pos = (chunk_length & 0xFF) * 2;
    hex[--idx] = lut[pos + 1];
    hex[--idx] = lut[pos];
  } while (chunk_length >>= 8);
  size_t begin = std::min(hex.find_first_not_of('0'), hex.length() - 3);  // keep at least 1 digit and "\r\n"
  return begin != std::string::npos ? hex.substr(begin) : hex;
}

}  // namespace trpc::stream
