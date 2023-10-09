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

#include "trpc/util/buffer/buffer.h"

#include <utility>

namespace trpc {

bool ContiguousToNonContiguous(BufferPtr& contiguous_buffer, NoncontiguousBuffer& noncontiguous_buffer) {
  size_t left_size = contiguous_buffer->ReadableSize();
  if (TRPC_UNLIKELY(left_size == 0)) {
    return false;
  }
  char* start = const_cast<char*>(contiguous_buffer->GetReadPtr());
  auto buf_blk = object_pool::MakeLwUnique<BufferBlock>();
  buf_blk->WrapUp(start, left_size);
  noncontiguous_buffer.Append(std::move(buf_blk));
  contiguous_buffer->DestructiveReset();
  return true;
}

bool NonContiguousToContiguous(const NoncontiguousBuffer& noncontiguous_buffer, BufferPtr& contiguous_buffer) {
  auto in_buffer_size = noncontiguous_buffer.ByteSize();
  if (in_buffer_size == 0) {
    return false;
  }
  if (!contiguous_buffer) {
    contiguous_buffer = MakeRefCounted<Buffer>(in_buffer_size);
  } else if (contiguous_buffer->WritableSize() < in_buffer_size) {
    contiguous_buffer->Resize(in_buffer_size);
  }
  auto itor = noncontiguous_buffer.begin();
  while (itor != noncontiguous_buffer.end()) {
    memcpy(contiguous_buffer->GetWritePtr(), itor->data(), itor->size());
    contiguous_buffer->AddWriteLen(itor->size());
    ++itor;
  }
  return true;
}

}  // namespace trpc
