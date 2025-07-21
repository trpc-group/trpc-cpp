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

#include "trpc/serialization/serialization_factory.h"

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::serialization {

SerializationFactory::SerializationFactory() {
  serializations_.resize(kMaxType);
  for (size_t i = 0; i < kMaxType; ++i) {
    serializations_[i] = nullptr;
  }
}

SerializationFactory::~SerializationFactory() {
  Clear();
}

bool SerializationFactory::Register(const SerializationPtr& serialization) {
  TRPC_ASSERT(serialization != nullptr);

  uint8_t type = serialization->Type();
  if (TRPC_UNLIKELY(type >= kMaxType)) {
    return false;
  }

  serializations_[type] = serialization;
  return true;
}

Serialization* SerializationFactory::Get(SerializationType serialization_type) {
  if (TRPC_UNLIKELY(serialization_type >= kMaxType)) {
    return nullptr;
  }

  return serializations_[serialization_type].Get();
}

void SerializationFactory::Clear() {
  for (size_t i = 0; i < serializations_.size(); ++i) {
    if (serializations_[i] != nullptr) {
      serializations_[i].Reset();
    }
  }
}

}  // namespace trpc::serialization
