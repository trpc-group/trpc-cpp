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

#include "trpc/serialization/flatbuffers/fbs_serialization.h"

#include <string>

#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/flatbuffers/message_fbs.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::serialization {

bool FbsSerialization::Serialize(DataType in_type, void* in, NoncontiguousBuffer* out) {
  TRPC_ASSERT(in_type == kFlatBuffers);

  auto* fbs_data = static_cast<flatbuffers::trpc::MessageFbs*>(in);
  uint32_t buff_size = fbs_data->ByteSizeLong();
  BufferPtr buffer_ptr = MakeRefCounted<Buffer>(buff_size);

  if (!fbs_data->SerializeToArray(buffer_ptr->GetWritePtr(), buff_size)) {
    return false;
  }

  buffer_ptr->AddWriteLen(buff_size);

  return ContiguousToNonContiguous(buffer_ptr, *out);
}

bool FbsSerialization::Deserialize(NoncontiguousBuffer* in, DataType out_type, void* out) {
  TRPC_ASSERT(out_type == kFlatBuffers);

  auto* fbs_data = static_cast<flatbuffers::trpc::MessageFbs*>(out);
  if (in->size() == 1) {
    BufferView buffer_view = in->FirstContiguous();
    if (!fbs_data->ParseFromArray(buffer_view.data(), buffer_view.size())) {
      TRPC_LOG_ERROR("flatbuffers deserialize failed");
      return false;
    }
  } else {
    std::string buffer = FlattenSlow(*in);
    if (!fbs_data->ParseFromArray(buffer.c_str(), buffer.size())) {
      TRPC_LOG_ERROR("flatbuffers deserialize failed");
      return false;
    }
  }

  return true;
}

}  // namespace trpc::serialization
