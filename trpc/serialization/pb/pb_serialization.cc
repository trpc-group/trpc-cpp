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

#include "trpc/serialization/pb/pb_serialization.h"

#include <utility>

#include "google/protobuf/message.h"

#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::serialization {

bool PbSerialization::Serialize(DataType in_type, void* in, NoncontiguousBuffer* out) {
  bool ret = false;
  switch (in_type) {
    case kPbMessage: {
      google::protobuf::Message* pb = static_cast<google::protobuf::Message*>(in);
      NoncontiguousBufferBuilder builder;
      NoncontiguousBufferOutputStream nbos(&builder);

      if (TRPC_UNLIKELY(!pb->SerializePartialToZeroCopyStream(&nbos))) {
        TRPC_LOG_ERROR("pb serialize failed");
        return ret;
      }

      nbos.Flush();
      *out = builder.DestructiveGet();
      ret = true;

      break;
    }
    case kNonContiguousBufferNoop: {
      NoncontiguousBuffer* buff = static_cast<NoncontiguousBuffer*>(in);
      *out = std::move(*buff);
      ret = true;

      break;
    }
    default: {
      TRPC_LOG_ERROR("serialization datatype:" << static_cast<int>(in_type)
                                               << " has not implement.");
    }
  }

  return ret;
}

bool PbSerialization::Deserialize(NoncontiguousBuffer* in, DataType out_type, void* out) {
  TRPC_ASSERT(out_type == kPbMessage);

  google::protobuf::Message* pb = static_cast<google::protobuf::Message*>(out);
  NoncontiguousBufferInputStream nbis(in);

  if (TRPC_UNLIKELY(!pb->ParseFromZeroCopyStream(&nbis))) {
    TRPC_LOG_ERROR("pb deserialize failed");
    return false;
  }

  nbis.Flush();

  return true;
}

}  // namespace trpc::serialization
