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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. flare
// Copyright (C) 2019 THL A29 Limited, a Tencent company.
// flare is licensed under the BSD 3-Clause License.
//

#pragma once

#include <cstddef>

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <numeric>
#include <string>
#include <utility>

#include "trpc/util/align.h"
#include "trpc/util/buffer/contiguous_buffer.h"
#include "trpc/util/buffer/memory_pool/memory_pool.h"
#include "trpc/util/check.h"
#include "trpc/util/internal/singly_linked_list.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

namespace detail {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer.h.

// Similar to `std::data()`, but the return type must be convertible to const char*.
template <class T>
constexpr auto data(const T& c)
    -> std::enable_if_t<std::is_convertible_v<decltype(c.data()), const char*>, const char*> {
  return c.data();
}

template <std::size_t N>
constexpr auto data(const char (&c)[N]) {
  return c;
}

template <class T>
constexpr std::size_t size(const T& c) {
  return c.size();
}

// Similar to `std::size` except for it returns number of chars instead of array
// size (which is 1 greater than number of chars.)
template <std::size_t N>
constexpr std::size_t size(const char (&c)[N]) {
  if (N == 0) {
    return 0;
  }
  return N - (c[N - 1] == 0);
}

}  // namespace detail

/// @brief Similar to `StringPiece`, used to view the memory pointed to by a pointer.
class BufferView {
 public:
  BufferView() noexcept : ptr_(nullptr), length_(0) {}
  BufferView(const char* buffer, std::size_t length) noexcept : ptr_(length ? buffer : nullptr), length_(length) {}

  /// @brief Returns a pointer to data kept by this buffer.
  /// @return The const char pointer of this buffer can be read
  const char* data() const noexcept { return ptr_; }

  /// @brief Returns the size of the buffer.
  /// @return The length of this buffer, which type is `std::size_t`
  std::size_t size() const noexcept { return length_; }

  /// @brief Skip a specified number of bytes.
  /// @param bytes The number of bytes skipped.
  void Skip(std::size_t bytes) noexcept {
    TRPC_DCHECK_LE(bytes, length_);
    ptr_ += bytes;
    length_ -= bytes;
  }

 private:
  const char* ptr_;     // Memory address.
  std::size_t length_;  // Memory size.
};

/// @brief Belongs to a middle layer object, mainly used by NoncontiguousBuffer. Each `BufferBlock` contains a
///        contiguous memory space, which can be a fixed-size memory block (`MemBlock`) allocated by the framework by
///        default, or a user-managed custom contiguous address space.
/// @note When using objects instantiated from this class, the following points should be noted:
/// 1、The type of memory address that can be processed is limited to either MemBlock or a contiguous memory block
///    passed in by the user. Mixing the two types is not allowed, which means that `data_` and `contiguous_buffer_`
///    cannot both be non-empty at the same time.
/// 2、Once `contiguous_buffer_` is managed by `WarpUp`, this interface cannot be called again to manage other
///    contiguous memory unless Clear is executed to allow calling this interface again.
/// 3、Once managed by the framework, the framework is responsible for releasing the contiguous memory, and the user
///    cannot continue to use the managed address separately.
struct BufferBlock {
  // Linked list node used for connecting memory blocks.
  internal::SinglyLinkedListEntry chain;

  BufferBlock() = default;
  BufferBlock(BufferBlock&& b) noexcept {
    start_ = std::exchange(b.start_, 0);
    size_ = std::exchange(b.size_, 0);
    data_ = std::move(b.data_);
    contiguous_buffer_ = std::move(b.contiguous_buffer_);
  }

  BufferBlock(const BufferBlock& b)
      : start_(b.start_), size_(b.size_), data_(b.data_), contiguous_buffer_(b.contiguous_buffer_) {}

  BufferBlock& operator=(BufferBlock&& b) {
    if (TRPC_UNLIKELY(&b == this)) {
      return *this;
    }
    start_ = std::exchange(b.start_, 0);
    size_ = std::exchange(b.size_, 0);
    data_ = std::move(b.data_);
    contiguous_buffer_ = std::move(b.contiguous_buffer_);
    return *this;
  }

  BufferBlock& operator=(const BufferBlock& b) {
    start_ = b.start_;
    size_ = b.size_;
    data_ = b.data_;
    contiguous_buffer_ = b.contiguous_buffer_;
    return *this;
  }

  /// @brief Accept a new memory block from framework.
  /// @param start Starting offset of a memory block.
  /// @param size Size of the memory block that can be operated on.
  /// @param data Memory block in the memory pool of framework.
  void Reset(std::size_t start, std::size_t size, RefPtr<memory_pool::MemBlock> data) {
    TRPC_DCHECK(!contiguous_buffer_, "contiguous buffer is not allowed to use in Reset ");
    start_ = start;
    size_ = size;
    data_ = std::move(data);
  }

  /// @brief Change the portion of buffer block we're seeing.
  /// @param bytes Number of bytes to skip.
  void Skip(std::size_t bytes) {
    TRPC_DCHECK_LT(bytes, size_);
    size_ -= bytes;
    start_ += bytes;
  }

  /// @brief Set the size of a buffer block.
  /// @param size Size of a buffer block.
  void SetSize(std::size_t size) {
    TRPC_DCHECK_LE(size, size_);
    size_ = size;
  }

  /// @brief Get the memory address pointer of the buffer block.
  /// @return char*
  /// @note The user needs to ensure that either `contiguous_buffer_` or `data_` is not empty.
  char* data() const noexcept {
    if (TRPC_LIKELY(data_)) {
      return data_->data + start_;
    }

    if (TRPC_UNLIKELY(contiguous_buffer_)) {
      return const_cast<char*>(contiguous_buffer_->GetReadPtr()) + start_;
    }

    return nullptr;
  }

  /// @brief Get the size of buffer block.
  /// @return The size of this buffer, which type is `std::size_t`
  std::size_t size() const noexcept { return size_; }

  /// @brief Clear the memory block.
  void Clear() {
    start_ = size_ = 0;
    if (TRPC_LIKELY(data_)) {
      data_.Reset();
    }
    if (TRPC_UNLIKELY(contiguous_buffer_)) {
      contiguous_buffer_.Reset();
    }
  }

  /// @brief Fully managed contiguous memory, where the object is responsible for releasing the managed memory.
  /// @param ptr The address of the managed contiguous memory.
  /// @param size The size of the managed contiguous memory.
  /// @note Once ptr is managed, the user cannot operate on it separately to avoid data errors. ptr must be allocated
  ///       through `new []`. this interface can only be used internally within the framework.
  void WrapUp(char* ptr, size_t size) {
    // If `data_` is already set, then `contiguous_buffer_` cannot be managed to avoid unexpected
    // issues that may arise from mixing them.
    TRPC_DCHECK(!data_, "contiguous buffer(`ptr`) cannot be mixed with memory blocks in noncontinuous buffers.");
    // The object only allows the `contiguous_buffer_` to be managed once.
    TRPC_DCHECK(!contiguous_buffer_, "Only allow to be wrapped up once for `ptr`.");
    contiguous_buffer_ = MakeRefCounted<ContiguousBuffer>(ptr, size);
    start_ = 0;
    size_ = size;
  }

  /// @brief Managed constant pointers are not supported at the moment.
  void WrapUp(const char* ptr, size_t size) { TRPC_ASSERT(false && "not allowing managed constant pointers."); }

  /// @brief Managed continuous buffer memory, after hosting,
  ///        the memory managed by continuous buffer will be moved
  /// @param contiguous_buffer Managed contiguous buffer
  /// @note `contiguous_buffer` no longer usable, this interface can only be used internally within the framework.
  void WrapUp(BufferPtr&& contiguous_buffer) {
    // The prerequisite for use is that both `data_` and `contiguous_buffer_` are empty.
    TRPC_DCHECK(!data_, "contiguous buffer(`BufferPtr`) cannot be mixed with memory blocks in noncontinuous buffers");
    TRPC_DCHECK(!contiguous_buffer_, "Only allow to be wrapped up once for `BufferPtr`.");

    contiguous_buffer_ = std::move(contiguous_buffer);
    start_ = 0;
    size_ = contiguous_buffer_->ReadableSize();
  }

 private:
  std::size_t start_{0}, size_{0};
  RefPtr<memory_pool::MemBlock> data_{nullptr};          // The framework provides a fixed-size contiguous memory block.
  RefPtr<ContiguousBuffer> contiguous_buffer_{nullptr};  // The contiguous space address passed in by the business.
};

/// @brief A helper class used to create `BufferBlock`.
class BufferBuilder {
 public:
  BufferBuilder();

  // Noncopyable / nonmovable.
  BufferBuilder(const BufferBuilder&) = delete;
  BufferBuilder& operator=(const BufferBuilder&) = delete;

  /// @brief Steal a buffer block of a specified byte size.
  /// @param bytes Number of bytes
  /// @return The object of `object_pool::LwUniquePtr<BufferBlock>`, which includes `bytes` bytes

  object_pool::LwUniquePtr<BufferBlock> Seal(std::size_t bytes) noexcept {
    auto rc = object_pool::MakeLwUnique<BufferBlock>();
    rc->Reset(used_, bytes, current_);
    used_ += bytes;
    if (used_ == GetBlockMaxAvailableSize()) {
      // If `current_` has been fully utilized, allocate a new BufferBlock
      AllocateBuffer();
    }
    return rc;
  }

  /// @brief Clear memory block.
  void Clear() { current_.Reset(); }

  /// @brief Get a pointer to where we could write.
  /// @return A `char` type pointer that supports both reading and writing.
  char* data() noexcept { return current_->data + used_; }

  /// @brief Maximum available memory size.
  /// @return The maximum size of availavle memory
  std::size_t SizeAvailable() const noexcept { return GetBlockMaxAvailableSize() - used_; }

 private:
  void AllocateBuffer();

 private:
  std::size_t used_;
  RefPtr<memory_pool::MemBlock> current_;
};

class NoncontiguousBuffer;

class NoncontiguousBoyerMooreSearcher {
 public:
  constexpr static size_t kAlphabetLen = 1 << std::numeric_limits<uint8_t>::digits;

  explicit NoncontiguousBoyerMooreSearcher(std::string_view pattern);
  explicit NoncontiguousBoyerMooreSearcher(std::string&& pattern)
      : NoncontiguousBoyerMooreSearcher(std::string_view{pattern}) {
    this->pattern_owned_ = std::move(pattern);
    this->pattern_ = pattern_owned_;
  }

  size_t operator()(const NoncontiguousBuffer& buffer) const noexcept;

 private:
  std::string pattern_owned_;
  std::string_view pattern_;
  std::array<ptrdiff_t, kAlphabetLen> delta1;
  std::unique_ptr<ptrdiff_t[]> delta2;
};

/// @brief Implementation of contiguous buffer
/// `NoncontiguousBuffer` is composed of multiple contiguous memory block `BufferBlock` objects,
/// Each BufferBlock contains a fixed-size contiguous memory block `MemBlock`.
/// In special scenarios, it can be degraded to continuous memory space, which only contains one BufferBlock.
/// This BufferBlock manages a continuous memory block of the size specified by the user.
/// @note When using this feature, there are several conditions that need to be taken into consideration:
/// 1、Once the NoncontiguousBuffer object has taken ownership of the user-specified continuous memory block,
///    the `buffers_` linked list of the object can only contain one BufferBlock object (which is used to wrap
///    the continuous memory block).
/// 2、Once the buffers_ linked list of the NoncontiguousBuffer object is not empty, it cannot take ownership of
///    continuous memory space.
class NoncontiguousBuffer {
  using LinkedBuffers = internal::SinglyLinkedList<BufferBlock, &BufferBlock::chain>;

 public:
  using iterator = LinkedBuffers::iterator;
  using const_iterator = LinkedBuffers::const_iterator;

  constexpr static size_t npos = -1;

  constexpr NoncontiguousBuffer() = default;

  NoncontiguousBuffer(const NoncontiguousBuffer& nb);
  NoncontiguousBuffer& operator=(const NoncontiguousBuffer& nb);

  NoncontiguousBuffer(NoncontiguousBuffer&& nb) noexcept
      : byte_size_(std::exchange(nb.byte_size_, 0)),
        buffers_(std::move(nb.buffers_)),
        is_contiguous_(std::exchange(nb.is_contiguous_, false)) {}

  NoncontiguousBuffer& operator=(NoncontiguousBuffer&& nb) noexcept {
    if (TRPC_LIKELY(&nb != this)) {
      Clear();
      std::swap(byte_size_, nb.byte_size_);
      buffers_ = std::move(nb.buffers_);
      std::swap(is_contiguous_, nb.is_contiguous_);
    }
    return *this;
  }

  ~NoncontiguousBuffer() { Clear(); }

  /// @brief To return the first continuous memory block in the linked list
  /// @return `BufferView` object contains information from the first continuous memory block in the linked list.
  BufferView FirstContiguous() const noexcept {
    TRPC_DCHECK(!Empty());
    auto&& first = buffers_.front();
    return {first.data(), first.size()};
  }

  /// @brief Get the position of the first occurrence of a search substring in the linked list
  /// @return The position of the first substring, or npos if it does not exist
  size_t Find(std::string_view s) const noexcept { return Find(NoncontiguousBoyerMooreSearcher(s)); }
  size_t Find(const NoncontiguousBoyerMooreSearcher& searcher) const noexcept { return searcher(*this); }

  /// @brief Skip a specified number of bytes in memory.
  /// @param bytes The number of bytes to skip.
  /// @note When the size to skip is greater than the size of the first intermediate BufferBlock,
  ///       multiple previous BufferBlock objects will be discarded.
  void Skip(std::size_t bytes) noexcept {
    TRPC_DCHECK_LE(bytes, ByteSize());
    if (TRPC_UNLIKELY(bytes == 0)) {
      // NOTHING.
    } else if (bytes < buffers_.front().size()) {
      buffers_.front().Skip(bytes);
      byte_size_ -= bytes;
    } else {
      SkipSlow(bytes);
    }
  }

  /// @brief Trim bytes bytes and create a new NoncontiguousBuffer object to be returned to the caller.
  /// @param bytes The number of bytes that were trimmed.
  /// @return Return the NoncontiguousBuffer object trimmed to the specified size to the user.
  /// @note `bytes` cannot be greater than the value returned by ByteSize().
  NoncontiguousBuffer Cut(std::size_t bytes) {
    TRPC_DCHECK_LE(bytes, ByteSize());

    if (bytes == ByteSize()) {
      return std::move(*this);
    }

    NoncontiguousBuffer rc;
    auto left = bytes;

    while (left && left >= buffers_.front().size()) {
      left -= buffers_.front().size();
      rc.buffers_.push_back(buffers_.pop_front());
    }

    if (TRPC_LIKELY(left)) {
      BufferBlock* ncb = object_pool::MakeLwUnique<BufferBlock>().Leak();
      *ncb = buffers_.front();
      ncb->SetSize(left);
      rc.buffers_.push_back(ncb);
      buffers_.front().Skip(left);
    }
    rc.byte_size_ = bytes;
    byte_size_ -= bytes;
    return rc;
  }

  /// @brief Add a contiguous memory block BufferBlock to the internal linked list.
  /// @param block A contiguous memory block BufferBlock
  /// @note Once the user has already handed over the ownership of a contiguous memory block,
  ///       this interface should not be used.
  void Append(object_pool::LwUniquePtr<BufferBlock>&& block) {
    TRPC_DCHECK(is_contiguous_ == false, "not supported in contigous mode");
    byte_size_ += block->size();
    buffers_.push_back(block.Leak());
  }

  /// @brief Add a non-contiguous memory NoncontiguousBuffer to the internal linked list.
  /// @param buffer A non-contiguous memory block NoncontiguousBuffer
  /// @note Once the user has already handed over the ownership of a contiguous memory block,
  ///       this interface should not be used.
  void Append(NoncontiguousBuffer buffer) {
    TRPC_DCHECK(is_contiguous_ == false, "not supported in contigous mode");
    byte_size_ += std::exchange(buffer.byte_size_, 0);
    buffers_.splice(std::move(buffer.buffers_));
  }

  /// @brief To fully manage the specified contiguous memory block provided by the user and have
  ///        the framework responsible for releasing the memory
  /// @param ptr The address of the managed contiguous memory block.
  /// @param size The size of the managed contiguous memory block.
  /// @note Once the memory block is managed, the user should avoid directly manipulating the `ptr` to prevent
  ///       unexpected issues. Additionally, the `ptr` must have been allocated using `new []` in order to be managed
  ///       by the framework.
  void Append(char* ptr, std::size_t size) {
    // If you want to add a contiguous buffer, it is required that the current size of buffers_ is 0,
    // and mixing different types of buffers is not supported.
    TRPC_DCHECK(Empty(), "only allow to include one contiguous buff");
    auto block = object_pool::MakeLwUnique<BufferBlock>();
    block->WrapUp(ptr, size);
    byte_size_ += block->size();
    buffers_.push_back(block.Leak());
    is_contiguous_ = true;
  }

  /// @brief Managed constant pointers are not supported at the moment.
  void Append(const char* ptr, std::size_t size) { TRPC_ASSERT(false && "not allowing managed constant pointers."); }

  /// @brief Manage the contiguous memory block specified by the user.
  /// @param contiguous_buff The object representing the managed contiguous memory block.
  /// @note Once the contiguous memory block is managed, the user should avoid directly manipulating `contiguous_buff`
  ///       to prevent unexpected issues.
  void Append(BufferPtr&& contiguous_buff) {
    // If you want to add a contiguous buffer, it is required that the current size of buffers_ is 0,
    // and mixing different types of buffers is not supported.
    TRPC_DCHECK(Empty(), "only allow to include one contiguous buff");
    size_t left_size = contiguous_buff->ReadableSize();
    if (TRPC_UNLIKELY(left_size == 0)) {
      TRPC_FMT_WARN("contiguous_buff len is 0");
      return;
    }
    auto block = object_pool::MakeLwUnique<BufferBlock>();
    block->WrapUp(std::move(contiguous_buff));
    byte_size_ += block->size();
    buffers_.push_back(block.Leak());
    is_contiguous_ = true;
  }

  /// @brief To obtain the total number of bytes contained in the managed object
  /// @return The size of memory block
  std::size_t ByteSize() const noexcept { return byte_size_; }

  /// @brief Check if the memory contained in the object is empty or not
  /// @return true: if the memory is empty; and false otherwise.
  bool Empty() const noexcept {
    TRPC_DCHECK_EQ(buffers_.empty(), byte_size_ == 0);
    return byte_size_ == 0;
  }

  /// @brief Clear the memory
  void Clear() noexcept {
    if (!Empty()) {
      ClearSlow();
    }
    is_contiguous_ = false;
  }

  // Iterator method
  constexpr iterator begin() noexcept { return buffers_.begin(); }
  constexpr iterator end() noexcept { return buffers_.end(); }

  constexpr const_iterator begin() const noexcept { return buffers_.begin(); }
  constexpr const_iterator end() const noexcept { return buffers_.end(); }

  /// @brief The number of BufferBlock objects contained in the managed object
  /// @return To return the number of BufferBlock objects
  std::size_t size() const noexcept { return buffers_.size(); }

 private:
  void SkipSlow(std::size_t bytes) noexcept;
  void ClearSlow() noexcept;

 private:
  std::size_t byte_size_{0};
  LinkedBuffers buffers_;
  bool is_contiguous_{false};
};

/// @brief The helper class used to construct a NoncontiguousBuffer object
class NoncontiguousBufferBuilder {
 public:
  NoncontiguousBufferBuilder() { InitializeNextBlock(); }

  /// @brief Get available addresses.
  /// @return Available address pointer.
  char* data() const noexcept { return current_->data + used_; }

  /// @brief Get maximum size of available memory.
  /// @return The maximum size of available memory.
  std::size_t SizeAvailable() const noexcept { return GetBlockMaxAvailableSize() - used_; }

  /// @brief Mark `bytes` bytes. If the current intermediate BufferBlock is fully utilized,
  ///        a new one will be constructed.
  /// @param bytes The number of bytes that have been marked.
  void MarkWritten(std::size_t bytes) {
    TRPC_DCHECK_LE(bytes, SizeAvailable(), "You're overflowing the buffer.");
    used_ += bytes;
    if (TRPC_UNLIKELY(!SizeAvailable())) {
      FlushCurrentBlock();
      InitializeNextBlock();
    }
  }

  /// @brief Reserve a byte memory of size `bytes` for the caller.
  /// @param bytes The number of bytes reserved.
  /// @return Return the memory address that has been reserved.
  /// @note The size(`bytes`) should not exceed the maximum available size of a single contiguous BufferBlock.
  char* Reserve(std::size_t bytes) {
    const std::size_t kMaxBytes = GetBlockMaxAvailableSize();

    TRPC_CHECK_LE(bytes, kMaxBytes, "At most [{}] bytes may be reserved in a single call.", kMaxBytes);
    if (SizeAvailable() < bytes) {
      // There is not enough space available in the intermediate contiguous buffer, so a new one needs to be created.
      FlushCurrentBlock();
      InitializeNextBlock();
    }
    auto* ptr = data();
    MarkWritten(bytes);
    return ptr;
  }

  /// @brief Get the number of bytes that are currently being used.
  /// @return The number of bytes that are currently being used.
  std::size_t ByteSize() const noexcept { return nb_.ByteSize() + used_; }

  /// @brief Return the constructed NoncontiguousBuffer to the caller.
  /// @return Return the NoncontiguousBuffer object
  /// @note After calling this method, do not use this builder anymore.
  NoncontiguousBuffer DestructiveGet() noexcept {
    FlushCurrentBlock();
    return std::move(nb_);
  }

  /// @brief Copy length bytes from the specified memory address `ptr` to the memory block of the managed object.
  /// @param ptr The specified memory address to be copied
  /// @param length The size of the bytes to be copied
  /// @note When the size of current_ is not enough, a new MemBlock will be constructed, and the copied data will be
  ///       stored in the managed object.
  void Append(const void* ptr, std::size_t length) {
    if (length == 0) {
      return;
    }

    auto current = data();
    // First, increase the value of `used_`. This operation may cause `used_` to temporarily overflow.
    // If it overflows, use the `AppendSlow` method to continue the operation.
    used_ += length;
    if (TRPC_LIKELY(used_ < GetBlockMaxAvailableSize())) {
      // If the current size of the intermediate contiguous buffer is sufficient, simply perform a direct copy.
#if __GNUC__ == 10
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
      memcpy(current, ptr, length);
#if __GNUC__ == 10
#pragma GCC diagnostic pop
#endif
      return;
    }
    used_ -= length;  // If the available space is not enough, you will need to use a slower copying method.
    AppendSlow(ptr, length);
  }

  /// @brief To add string memory data to the memory block of the managed object.
  /// @param s The object of string
  void Append(const std::string_view& s) { return Append(s.data(), s.size()); }

  /// @brief Add BufferView memory data to the memory block of the managed object.
  /// @param v BufferView object
  void Append(BufferView v) { return Append(v.data(), v.size()); }

  /// @brief Add the memory data from NoncontiguousBuffer to be stored in the memory block of the managed object.
  /// @param buffer A NoncontiguousBuffer object
  void Append(NoncontiguousBuffer buffer) {
    // If there have been no operations on the intermediate contiguous cache, there is no need to execute this step.
    if (used_) {
      FlushCurrentBlock();
      InitializeNextBlock();
    }
    nb_.Append(std::move(buffer));
  }

  /// @brief Add a single byte to the memory block of the managed object by copy.
  /// @param c A single byte that is stored.
  void Append(char c) { Append(static_cast<unsigned char>(c)); }

  /// @brief Adding a single byte to the managed object.
  /// @param c The stored single byte.
  void Append(unsigned char c) {
    TRPC_DCHECK(SizeAvailable());  // Check the available space.
    *reinterpret_cast<unsigned char*>(data()) = c;
    MarkWritten(1);
  }

  /// @brief Mainly used to add small objects to the already allocated buffer,
  ///        where Ts::data() and Ts::size() must be of a usable type.
  /// @param buffers Memory object
  template <class... Ts, class = std::void_t<decltype(detail::data(std::declval<Ts&>()))...>,
            class = std::void_t<decltype(detail::size(std::declval<Ts&>()))...>,
            class = std::enable_if_t<(sizeof...(Ts) > 1)>>
  void Append(const Ts&... buffers) {
    auto current = data();
    auto total = (detail::size(buffers) + ...);
    used_ += total;
    if (TRPC_LIKELY(used_ < GetBlockMaxAvailableSize())) {
      UncheckedAppend(current, buffers...);
      return;
    }

    used_ -= total;
    [[maybe_unused]] int dummy[] = {(Append(buffers), 0)...};
  }

 private:
  // Allocate a new contiguous buffer.
  void InitializeNextBlock();

  // Move the currently used contiguous buffer to NoncontiguousBuffer.
  void FlushCurrentBlock();

  void AppendSlow(const void* ptr, std::size_t length);

  template <class T, class... Ts>
  [[gnu::always_inline]] void UncheckedAppend(char* ptr, const T& sv) {
    memcpy(ptr, detail::data(sv), detail::size(sv));
  }

  template <class T, class... Ts>
  [[gnu::always_inline]] void UncheckedAppend(char* ptr, const T& sv, const Ts&... svs) {
    memcpy(ptr, detail::data(sv), detail::size(sv));
    UncheckedAppend(ptr + detail::size(sv), svs...);
  }

 private:
  NoncontiguousBuffer nb_;
  std::size_t used_{0};
  RefPtr<memory_pool::MemBlock> current_;
};

// The following are helper functions that should not be used directly by the user.
namespace detail {

// Copy the contents from the NoncontiguousBuffer object to the specified address.
void FlattenToSlowSlow(const NoncontiguousBuffer& nb, void* buffer, std::size_t size);
}  // namespace detail

/// @brief Copy the contents of the string to a BufferBlock object.
/// @param s A specified string object needed copied
/// @return A BufferBlock object included content of `s`
object_pool::LwUniquePtr<BufferBlock> CreateBufferBlockSlow(std::string_view s);

/// @brief Copy the contents of size `size` from `ptr` to a BufferBlock object.
/// @param ptr The address of the data being copied.
/// @param size The size of the data being copied.
/// @return BufferBlock object included content of `ptr`
object_pool::LwUniquePtr<BufferBlock> CreateBufferBlockSlow(const void* ptr, std::size_t size);

/// @brief Copy the contents of the string to a NoncontiguousBuffer object.
/// @param s The string being copied.
/// @return A NoncontiguousBuffer object that stores data of `s`
NoncontiguousBuffer CreateBufferSlow(std::string_view s);

/// @brief Copy the contents of `ptr` of size `size` to the NoncontiguousBuffer object.
/// @param ptr The address of the data being copied.
/// @param size The size of the data being copied.
/// @return A NoncontiguousBuffer object that stores data of size `size` from `ptr`.
NoncontiguousBuffer CreateBufferSlow(const void* ptr, std::size_t size);

/// @brief Copy the contents of the NoncontiguousBuffer to a string object.
/// @param nb The NoncontiguousBuffer object being copied.
/// @param max_bytes The maximum size of the data being copied.
/// @return A string object that contains the data content of `max_bytes` bytes from `nb`.
std::string FlattenSlow(const NoncontiguousBuffer& nb, std::size_t max_bytes = std::numeric_limits<std::size_t>::max());

/// @brief Search for the content of delim in nb. If found, compare the position of the found content plus the size of
///        delim with max_bytes and take the smaller value. Otherwise, copy the data of size `max_bytes` to a string and
///        return it to the user.
/// @param nb The NoncontiguousBuffer object being copied.
/// @param delim The target string being searched.
/// @param max_bytes he maximum size of the data being copied.
/// @return A string converted from a `nb` that contains the content of `delim`.
std::string FlattenSlowUntil(const NoncontiguousBuffer& nb, std::string_view delim,
                             std::size_t max_bytes = std::numeric_limits<std::size_t>::max());

/// @brief Copy size bytes from NoncontiguousBuffer to contiguous buffer.
/// @param nb The source address of the data being copied.
/// @param buffer The destination address of the data being copied.
/// @param size The size of the data being copied.
/// @note The caller ensures that `nb.ByteSize()` is greater than `size`.
inline void FlattenToSlow(const NoncontiguousBuffer& nb, void* buffer, std::size_t size) {
  if (TRPC_LIKELY(size <= nb.FirstContiguous().size())) {
    memcpy(buffer, nb.FirstContiguous().data(), size);
    return;
  }
  return detail::FlattenToSlowSlow(nb, buffer, size);
}

}  // namespace trpc

namespace trpc {

template <>
struct object_pool::ObjectPoolTraits<BufferBlock> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace trpc
