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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "trpc/compressor/compressor.h"
#include "trpc/compressor/compressor_type.h"

namespace trpc::compressor {

/// @brief A factory for compression plugins.
/// It is ok for a compression plugin is registered to or got from the factory.
/// @note Registry operation is not thread-safe. Always keep in mind: compression plugins must be registered when the
/// program is initialized.
class CompressorFactory {
 public:
  ~CompressorFactory();

  /// @brief Gets the instance of factory.
  /// @return Returns the pointer of "CompressorFactory".
  static CompressorFactory* GetInstance() {
    static CompressorFactory instance;
    return &instance;
  }

  /// @brief Registers a compression plugin instance who implements the interface "Compressor"  to the factory.
  /// @param compressor is a pointer of compression plugin instance "Compressor".
  /// @return Returns true on success, false otherwise.
  bool Register(const CompressorPtr& compressor);

  /// @brief Gets the compression plugin instance according to the type of compression type.
  /// @param compress_type is compression type.
  /// @return Returns a pointer of compression plugin instance on success, nullptr otherwise.
  Compressor* Get(CompressType compress_type);

  /// @brief Does some cleanup job.
  void Clear();

 private:
  CompressorFactory();
  CompressorFactory(const CompressorFactory&) = delete;
  CompressorFactory& operator=(const CompressorFactory&) = delete;

 private:
  // Default: nullptr is at compressor::kNone.
  std::vector<CompressorPtr> compressors_;
};

}  // namespace trpc::compressor
