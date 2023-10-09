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

#include <vector>

#include "trpc/serialization/serialization.h"

namespace trpc::serialization {

/// @brief Plugin factory class for serialization and deserialization of business layer data
/// @note  `Register` interface is not thread-safe and needs to be called when the program starts
class SerializationFactory {
 public:
  static SerializationFactory* GetInstance() {
    static SerializationFactory instance;
    return &instance;
  }

  ~SerializationFactory();

  /// @brief Register serialization and deserialization plugin
  bool Register(const SerializationPtr& serialization);

  /// @brief Get serialization and deserialization plugin by type
  Serialization* Get(SerializationType serialization_type);

  /// @brief Clear registered serialization plugins
  void Clear();

 private:
  SerializationFactory();
  SerializationFactory(const SerializationFactory&) = delete;
  SerializationFactory& operator=(const SerializationFactory&) = delete;

 private:
  std::vector<SerializationPtr> serializations_;
};

}  // namespace trpc::serialization
