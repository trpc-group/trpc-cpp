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

#include "trpc/serialization/noop/noop_serialization.h"

#include <string>
#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc::serialization {

bool NoopSerialization::Serialize(DataType in_type, void* in, NoncontiguousBuffer* out) {
  bool ret = false;
  switch (in_type) {
    case kStringNoop:
      {
        std::string* str = static_cast<std::string*>(in);
        *out = CreateBufferSlow(*str);
        ret = true;
      }
      break;
    case kNonContiguousBufferNoop:
      {
        NoncontiguousBuffer* buff = static_cast<NoncontiguousBuffer*>(in);
        *out = std::move(*buff);
        ret = true;
      }
      break;
    default:
      {
        TRPC_LOG_ERROR("serialization datatype:" << static_cast<int>(in_type) <<
                       ", has not implement.");
      }
  }

  return ret;
}

bool NoopSerialization::Deserialize(NoncontiguousBuffer* in, DataType out_type, void* out) {
  bool ret = false;
  switch (out_type) {
    case kStringNoop:
      {
        std::string* str = static_cast<std::string*>(out);
        *str = FlattenSlow(*in);
        ret = true;
      }
      break;
    case kNonContiguousBufferNoop:
      {
        NoncontiguousBuffer* buff = static_cast<NoncontiguousBuffer*>(out);
        *buff = std::move(*in);
        ret = true;
      }
      break;
    default:
      {
        TRPC_LOG_ERROR("serialization datatype:" << static_cast<int>(out_type) <<
                       ", has not implement.");
      }
  }

  return ret;
}

}  // namespace trpc::serialization
