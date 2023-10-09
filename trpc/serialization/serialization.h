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

#include <cstdint>
#include <memory>

#include "trpc/serialization/serialization_type.h"
#include "trpc/util/buffer/buffer.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Namespace of serialization implementation.
namespace serialization {

/// @brief Base class for serialization and deserialization of business data
class Serialization : public RefCounted<Serialization> {
 public:
  virtual ~Serialization() = default;

  /// @brief The type of serialization and deserialization.
  /// @note  All type need to be defined in `seriliazation_type.h`.
  virtual SerializationType Type() const = 0;

  /// @brief Serialization of business data
  /// @param `in_type` is data type of the `in`
  /// @param `in` is the data structure object that business data needs to encode
  /// @param `out` is serialized binary data
  /// @return true: success, false: failed
  virtual bool Serialize(DataType in_type, void* in, NoncontiguousBuffer* out) = 0;

  /// @brief Deserialization of business data
  /// @param `in` is the binary data that needs to be deserialized
  /// @param `out_type` is data type of the `out`
  /// @param `out` is deserialized object
  /// @return true: success, false: failed
  virtual bool Deserialize(NoncontiguousBuffer* in, DataType out_type, void* out) = 0;
};

using SerializationPtr = RefPtr<Serialization>;

}  // namespace serialization

}  // namespace trpc
