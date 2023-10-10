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

#pragma once
// Helper functionality to glue FlatBuffers and TRPC.

#include <assert.h>
#include <memory>
#include <utility>

#include "flatbuffers/flatbuffers.h"

#include "message_fbs.h"

#include "trpc/util/buffer/buffer.h"
#include "trpc/util/buffer/contiguous_buffer.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/likely.h"
#include "trpc/util/ref_ptr.h"

namespace flatbuffers {
namespace trpc {

/// @brief  Message is a typed wrapper around a buffer that manages the underlying
/// `slice` and also provides flatbuffers-specific helpers such as `Verify`
/// and `GetRoot`. Since it is backed by a `slice`, the underlying buffer
/// is ref-counted and ownership is be managed automatically.
template <class T>
class Message : public MessageFbs {
 public:
  explicit Message(size_t len = 0) { slice_ = ::trpc::MakeRefCounted<::trpc::Buffer>(len); }

  explicit Message(::trpc::BufferPtr& buff) : slice_(buff) {}

  Message(const Message& other) { slice_ = other.slice_; }

  Message& operator=(const Message& other) {
    if (slice_.Get() == other.slice_.Get()) {
      return *this;
    }
    slice_ = other.slice_;
    return *this;
  }

  Message(Message&& other) {
    slice_ = std::move(other.slice_);
    other.slice_ = ::trpc::MakeRefCounted<::trpc::Buffer>(0);
  }

  Message& operator=(Message&& other) {
    if (this != &other) {
      slice_ = std::move(other.slice_);
      other.slice_ = ::trpc::MakeRefCounted<::trpc::Buffer>(0);
    }
    return *this;
  }

  const uint8_t* mutable_data() const override { return reinterpret_cast<const uint8_t*>(slice_->GetReadPtr()); }

  const uint8_t* data() const override { return reinterpret_cast<const uint8_t*>(slice_->GetReadPtr()); }

  size_t size() const override { return slice_->ReadableSize(); }

  // For compatibility (trpc:rpc_method_handler.h:109:38).
  uint32_t ByteSizeLong() const override { return size(); }

  bool Verify() const {
    Verifier verifier(data(), size());
    return verifier.VerifyBuffer<T>(nullptr);
  }

  T* GetMutableRoot() {
    uint8_t* md = const_cast<uint8_t*>(mutable_data());
    return flatbuffers::GetMutableRoot<T>(md);
  }

  const T* GetRoot() const { return flatbuffers::GetRoot<T>(data()); }

  // This is only intended for serializer use, or if you know what you're doing
  const ::trpc::BufferPtr& BorrowSlice() const { return slice_; }

  bool SerializeToArray(char* arr, uint32_t len) const override {
    if (TRPC_UNLIKELY(size() != len)) {
      return false;
    }
    memcpy(arr, data(), len);
    return true;
  }

  bool ParseFromArray(const char* arr, uint32_t len) override {
    bool ret = true;
    slice_->Resize(len);
    memcpy(slice_->GetWritePtr(), arr, len);
    slice_->AddWriteLen(len);
#ifndef FLATBUFFERS_TRPC_DISABLE_AUTO_VERIFICATION
    ret = Verify();
#endif
    return ret;
  }

 private:
  ::trpc::BufferPtr slice_;
};

class MessageBuilder;

/// @brief SliceAllocator is a  allocator that uses the `slice` ref-counted slices to manage memory ownership.
/// @note This makes it easy and efficient to transfer buffers to tRPC.
/// @private For internal use purpose only.
class SliceAllocator : public Allocator {
 public:
  SliceAllocator();

  virtual ~SliceAllocator();

  SliceAllocator(const SliceAllocator& other) = delete;
  SliceAllocator& operator=(const SliceAllocator& other) = delete;

  SliceAllocator(SliceAllocator&& other) {
    // default-construct and swap idiom
    swap(other);
  }
  SliceAllocator& operator=(SliceAllocator&& other) {
    // move-construct and swap idiom
    SliceAllocator temp(std::move(other));
    swap(temp);
    return *this;
  }

  void swap(SliceAllocator& other) {
    using std::swap;
    swap(slice_, other.slice_);
  }

  uint8_t* allocate(size_t size) override;

  void deallocate(uint8_t* p, size_t size) override;

  uint8_t* reallocate_downward(uint8_t* old_p, size_t old_size, size_t new_size, size_t in_use_back,
                               size_t in_use_front) override;

 private:
  ::trpc::BufferPtr& GetSlice(uint8_t* p, size_t size) {
    CheckSlice(p, size);
    return slice_;
  }

  void CheckSlice(uint8_t* p, size_t size) {
    TRPC_ASSERT(p == reinterpret_cast<const uint8_t*>(slice_->GetReadPtr()));
    TRPC_ASSERT(size == slice_->ReadableSize());
  }

  void CreateBuffer(uint8_t* buf, size_t len) {
    if (buf != nullptr && len > 0) {
      slice_ = ::trpc::MakeRefCounted<::trpc::Buffer>(len);
      memcpy(slice_->GetWritePtr(), buf, len);
      slice_->AddWriteLen(len);
    } else {
      slice_ = ::trpc::MakeRefCounted<::trpc::Buffer>(reinterpret_cast<char*>(buf), len);
    }
  }

 private:
  ::trpc::BufferPtr slice_;

  friend class MessageBuilder;
};

namespace detail {
// @brief SliceAllocatorMember is a hack to ensure that the MessageBuilder's
// slice_allocator_ member is constructed before the FlatBufferBuilder, since
// the allocator is used in the FlatBufferBuilder ctor.
struct SliceAllocatorMember {
  SliceAllocator slice_allocator_;
};
}  // namespace detail

/// @brief MessageBuilder is a specific FlatBufferBuilder that uses SliceAllocator to allocate tRPC buffers.
class MessageBuilder : private detail::SliceAllocatorMember, public FlatBufferBuilder {
 public:
  explicit MessageBuilder(uoffset_t initial_size = 1024);

  explicit MessageBuilder(FlatBufferBuilder&& src, void (*dealloc)(void*, size_t) = &DefaultAllocator::dealloc);

  ~MessageBuilder() {}

  MessageBuilder(const MessageBuilder& other) = delete;
  MessageBuilder& operator=(const MessageBuilder& other) = delete;

  MessageBuilder(MessageBuilder&& other) : FlatBufferBuilder(1024, &slice_allocator_, false) {
    // Default construct and swap idiom.
    Swap(other);
  }

  // Move-assign a FlatBufferBuilder to a MessageBuilder.
  // Only FlatBufferBuilder with default allocator (basically, nullptr) is
  // supported.
  MessageBuilder& operator=(FlatBufferBuilder&& src) {
    // Move construct a temporary and swap
    MessageBuilder temp(std::move(src));
    Swap(temp);
    return *this;
  }

  MessageBuilder& operator=(MessageBuilder&& other) {
    // Move construct a temporary and swap
    MessageBuilder temp(std::move(other));
    Swap(temp);
    return *this;
  }

  void Swap(MessageBuilder& other) {
    slice_allocator_.swap(other.slice_allocator_);
    FlatBufferBuilder::Swap(other);
    // After swapping the FlatBufferBuilder, we swap back the allocator, which
    // restores the original allocator back in place. This is necessary because
    // MessageBuilder's allocator is its own member (SliceAllocatorMember). The
    // allocator passed to FlatBufferBuilder::vector_downward must point to this
    // member.
    buf_.swap_allocator(other.buf_);
  }

  /// @brief Releases the ownership of the buffer pointer.
  /// @return Returns the size, offset, and the original slice that
  /// allocated the buffer.
  uint8_t* ReleaseRaw(size_t& size, size_t& offset, ::trpc::BufferPtr& slice);

  /// @brief Extracts the subslice of the buffer corresponding to the
  /// flatbuffers-encoded region and wraps it in a `Message<T>` to handle buffer
  /// ownership.
  template <class T>
  Message<T> GetMessage() {
    auto buf_data = buf_.scratch_data();  // pointer to memory
    auto buf_size = buf_.capacity();      // size of memory
    auto msg_data = buf_.data();          // pointer to msg
    auto msg_size = buf_.size();          // size of msg
    // Do some sanity checks on data/size
    TRPC_ASSERT(msg_data);
    TRPC_ASSERT(msg_size);
    TRPC_ASSERT(msg_data >= buf_data);
    TRPC_ASSERT(msg_data + msg_size <= buf_data + buf_size);
    // Calculate offsets from the buffer start
    auto begin = msg_data - buf_data;
    auto end = begin + msg_size;
    // Get the slice we are working with (no refcount change)
    slice_allocator_.CheckSlice(buf_data, buf_size);
    TRPC_ASSERT(end >= begin);

    auto subslice = ::trpc::MakeRefCounted<::trpc::Buffer>(msg_size);
    memcpy(subslice->GetWritePtr(), msg_data, msg_size);
    subslice->AddWriteLen(msg_size);
    // Wrap the subslice in a `Message<T>`, but don't increment refcount
    Message<T> msg(subslice);
    return msg;
  }

  template <class T>
  Message<T> ReleaseMessage() {
    Message<T> msg = GetMessage<T>();
    Reset();
    return msg;
  }
};

}  // namespace trpc
}  // namespace flatbuffers
