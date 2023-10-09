//
// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
// Copy and modify from https://github.com/google/flatbuffers/blob/v23.5.26/include/flatbuffers/grpc.h.

#include "trpc/util/flatbuffers/trpc_fbs.h"

namespace flatbuffers {
namespace trpc {

SliceAllocator::SliceAllocator() : slice_(::trpc::MakeRefCounted<::trpc::Buffer>(0)) {}

SliceAllocator::~SliceAllocator() {}

uint8_t* SliceAllocator::allocate(size_t size) {
  TRPC_ASSERT(slice_->WritableSize() == 0);
  slice_->Resize(size);
  auto* ptr = reinterpret_cast<uint8_t*>(slice_->GetWritePtr());
  slice_->AddWriteLen(size);
  return ptr;
}

void SliceAllocator::deallocate(uint8_t* p, size_t size) {
  TRPC_ASSERT(reinterpret_cast<char*>(p) == slice_->GetReadPtr());
  TRPC_ASSERT(size == slice_->ReadableSize());
  slice_ = ::trpc::MakeRefCounted<::trpc::Buffer>(0);
}

uint8_t* SliceAllocator::reallocate_downward(uint8_t* old_p, size_t old_size, size_t new_size, size_t in_use_back,
                                             size_t in_use_front) {
  TRPC_ASSERT(reinterpret_cast<char*>(old_p) == slice_->GetReadPtr());
  TRPC_ASSERT(old_size == slice_->ReadableSize());
  TRPC_ASSERT(new_size > old_size);
  ::trpc::BufferPtr new_slice = ::trpc::MakeRefCounted<::trpc::Buffer>(new_size);
  uint8_t* new_p = reinterpret_cast<uint8_t*>(new_slice->GetWritePtr());
  memcpy_downward(old_p, old_size, new_p, new_size, in_use_back, in_use_front);
  new_slice->AddWriteLen(new_size);
  slice_ = new_slice;
  return new_p;
}

// MessageBuilder
MessageBuilder::MessageBuilder(uoffset_t initial_size) : FlatBufferBuilder(initial_size, &slice_allocator_, false) {}

MessageBuilder::MessageBuilder(FlatBufferBuilder&& src, void (*dealloc)(void*, size_t))
    : FlatBufferBuilder(1024, &slice_allocator_, false) {
  src.Swap(*this);
  src.SwapBufAllocator(*this);
  if (buf_.capacity()) {
    uint8_t* buf = buf_.scratch_data();  // pointer to memory
    size_t capacity = buf_.capacity();   // size of memory
    slice_allocator_.CreateBuffer(buf, capacity);
  } else {
    slice_allocator_.CreateBuffer(nullptr, 0);
  }
}

uint8_t* MessageBuilder::ReleaseRaw(size_t& size, size_t& offset, ::trpc::BufferPtr& slice) {
  uint8_t* buf = FlatBufferBuilder::ReleaseRaw(size, offset);
  slice = slice_allocator_.slice_;
  slice_allocator_.CreateBuffer(nullptr, 0);
  return buf;
}
}  // namespace trpc
}  // namespace flatbuffers
