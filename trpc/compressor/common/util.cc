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

#include "trpc/compressor/common/util.h"

#include <cstring>

#include "trpc/util/log/logging.h"

namespace trpc::compressor {

bool CopyDataToOutputStream(const void* data, std::size_t size, trpc::NoncontiguousBufferOutputStream* out) {
  if (size == 0) {
    return true;
  }

  std::size_t current_pos = 0;
  int left_to_copy = static_cast<int>(size);
  while (true) {
    void* next_data;
    int next_size;
    // New a buffer block to write.
    out->Next(&next_data, &next_size);

    if (left_to_copy <= next_size) {
      // Copy full content.
      memcpy(next_data, reinterpret_cast<const char*>(data) + current_pos, left_to_copy);
      out->BackUp(next_size - left_to_copy);
      return true;
    } else {
      // Copy partial content.
      memcpy(next_data, reinterpret_cast<const char*>(data) + current_pos, next_size);
      current_pos += next_size;
      left_to_copy -= next_size;
    }
  }
}

}  // namespace trpc::compressor
