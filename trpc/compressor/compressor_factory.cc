//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/compressor/compressor_factory.h"

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::compressor {

CompressorFactory::CompressorFactory() {
  compressors_.resize(kMaxType);
  for (std::size_t i = 0; i < kMaxType; ++i) {
    compressors_[i] = nullptr;
  }
}

CompressorFactory::~CompressorFactory() { Clear(); }

bool CompressorFactory::Register(const CompressorPtr& compressor) {
  TRPC_ASSERT(compressor != nullptr);

  auto type = compressor->Type();
  if (TRPC_UNLIKELY(type >= kMaxType)) {
    return false;
  }
  compressors_[type] = compressor;
  return true;
}

Compressor* CompressorFactory::Get(CompressType compress_type) {
  if (TRPC_UNLIKELY(compress_type >= kMaxType)) {
    return nullptr;
  }
  return compressors_[compress_type].get();
}

void CompressorFactory::Clear() {
  for (auto&& ptr : compressors_) {
    if (ptr) {
      ptr.Reset();
    }
  }}

}  // namespace trpc::compressor
