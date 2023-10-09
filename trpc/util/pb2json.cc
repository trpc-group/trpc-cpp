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

#include "trpc/util/pb2json.h"

namespace trpc {

bool Pb2Json::PbToJson(const google::protobuf::Message& message, std::string* output) {
  output->clear();

  google::protobuf::util::JsonPrintOptions opts;

  // Whether to always print enums as ints.
  // By default they are rendered as strings.
  opts.always_print_enums_as_ints = true;
  // Whether to preserve proto field names
  opts.preserve_proto_field_names = true;
  // Whether to always print primitive fields. By default proto3 primitive
  // fields with default values will be omitted in JSON output. For example, an
  // int32 field set to 0 will be omitted. Set this flag to true will override
  // the default behavior and print primitive fields regardless of their values.
  opts.always_print_primitive_fields = true;

  google::protobuf::util::Status status =
      google::protobuf::util::MessageToJsonString(message, output, opts);

  return status.ok();
}

bool Pb2Json::JsonToPb(const std::string& json, google::protobuf::Message* message) {
  google::protobuf::util::JsonParseOptions opts;

  // Whether to ignore unknown json fields,
  // the default is not ignored and an error occurs if there are unknown fields
  opts.ignore_unknown_fields = true;

  google::protobuf::util::Status status =
      google::protobuf::util::JsonStringToMessage(json, message, opts);

  return status.ok();
}

bool Pb2Json::JsonToPb(const std::string_view& json, google::protobuf::Message* message) {
  google::protobuf::StringPiece string_piece(json.data(), json.size());

  google::protobuf::util::JsonParseOptions opts;

  // Whether to ignore unknown json fields,
  // the default is not ignored and an error occurs if there are unknown fields
  opts.ignore_unknown_fields = true;

  google::protobuf::util::Status status =
      google::protobuf::util::JsonStringToMessage(string_piece, message, opts);

  return status.ok();
}

}  // namespace trpc
