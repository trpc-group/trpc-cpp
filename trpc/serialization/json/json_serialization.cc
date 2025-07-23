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

#include "trpc/serialization/json/json_serialization.h"

#include <string>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/pb2json.h"

namespace trpc::serialization {

bool JsonSerialization::Serialize(DataType in_type, void* in, NoncontiguousBuffer* out) {
  bool ret = false;
  switch (in_type) {
    case kRapidJson: {
      rapidjson::Document* rapidjson_doc = static_cast<rapidjson::Document*>(in);
      if (!rapidjson_doc->IsNull()) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        rapidjson_doc->Accept(writer);

        std::string_view data(buffer.GetString(), buffer.GetSize());

        *out = CreateBufferSlow(data);
      }

      ret = true;
    } break;
    case kPbMessage: {
      google::protobuf::Message* pb = static_cast<google::protobuf::Message*>(in);
      std::string json;
      ret = Pb2Json::PbToJson(*pb, &json);
      if (ret) {
        *out = CreateBufferSlow(json);
      } else {
        TRPC_LOG_ERROR("PbToJson Failed.");
      }
    } break;
    default: {
      TRPC_LOG_ERROR("serialization datatype:" << static_cast<int>(in_type) << " has not implement.");
    }
  }

  return ret;
}

bool JsonSerialization::Deserialize(NoncontiguousBuffer* in, DataType out_type, void* out) {
  std::string buffer = FlattenSlow(*in);
  bool ret = false;

  switch (out_type) {
    case kRapidJson: {
      rapidjson::Document* rapidjson_doc = static_cast<rapidjson::Document*>(out);
      rapidjson_doc->Parse(buffer.c_str(), buffer.size());
      if (!buffer.empty() && rapidjson_doc->HasParseError()) {
        TRPC_LOG_ERROR("JsonParse Failed:" << rapidjson::GetParseError_En(rapidjson_doc->GetParseError()));
      } else {
        ret = true;
      }
    } break;
    case kPbMessage: {
      auto request_data = std::string_view(buffer.c_str(), buffer.size());
      google::protobuf::Message* pb = static_cast<google::protobuf::Message*>(out);

      ret = Pb2Json::JsonToPb(request_data, pb);
      if (!ret) {
        TRPC_LOG_ERROR("JsonToPb Failed.");
      }
    } break;
    default: {
      TRPC_LOG_ERROR("serialization datatype:" << static_cast<int>(out_type) << " has not impliment.");
    }
  }

  return ret;
}

}  // namespace trpc::serialization
