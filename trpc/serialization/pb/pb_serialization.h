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

#include "trpc/serialization/serialization.h"

namespace trpc::serialization {

/// @brief Implementation of serialization and deserialization based on protobuf
class PbSerialization : public Serialization {
 public:
  ~PbSerialization() override = default;

  SerializationType Type() const { return kPbType; }

  bool Serialize(DataType in_type, void* in, NoncontiguousBuffer* out) override;

  bool Deserialize(NoncontiguousBuffer* in, DataType out_type, void* out) override;
};

}  // namespace trpc::serialization
