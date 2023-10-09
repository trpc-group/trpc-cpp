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

#include <string>

#include "google/protobuf/util/json_util.h"

namespace trpc {

/// @brief Implementation of conversion between pb and json
class Pb2Json {
 public:
  /// @brief pb to json
  static bool PbToJson(const google::protobuf::Message& message, std::string* output);

  /// @brief json to pb
  static bool JsonToPb(const std::string& json, google::protobuf::Message* message);

  /// @brief json to pb
  static bool JsonToPb(const std::string_view& json, google::protobuf::Message* message);
};

}  // namespace trpc
