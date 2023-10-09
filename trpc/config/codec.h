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

#include <any>
#include <string>
#include <unordered_map>

#include "trpc/common/plugin.h"

namespace trpc::config {

/// @brief The codec class is an abstract interface definition class for config data codecs.
class Codec : public Plugin {
 public:
  /// @brief  Plug-in Types
  /// @return Return plug-in type
  PluginType Type() const override { return PluginType::kConfigCodec; }

  /// @brief Returns the codec's name.
  /// @return The name of the codec.
  virtual std::string Name() const = 0;

  /// @brief Deserializes the config data bytes into the second input parameter.
  /// @param data The config data bytes to deserialize.
  /// @param out The output parameter to store the deserialized config data.
  /// @return True if the deserialization was successful, false otherwise.
  virtual bool Decode(const std::string& data, std::unordered_map<std::string, std::any>& out) const = 0;
};

using CodecPtr = RefPtr<Codec>;

}  // namespace trpc::config
