//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/detail/writing_buffer_list.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <vector>

#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/util/align.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

namespace object_pool {

template <class>
struct ObjectPoolTraits;

}  // namespace object_pool

/// @brief An mpsc writing buffer queue.
class alignas(hardware_destructive_interference_size) WritingBufferList {
 public:
  enum BufferAppendStatus { kAppendHead, kAppendTail, kTimeout };

  WritingBufferList() = default;

  ~WritingBufferList();

  /// @brief Use writev to send data
  /// @param[in] io io handler
  /// @param[in] conn_handler connection handler
  /// @param[in] max_bytes the maximum number of bytes sent at a time
  /// @param[in] max_capacity the maximum number of bytes sent at a time
  /// @param[in] support_pipeline whether to send the package in an orderly pipeline
  /// @param[out] emptied whether the data has all been sent
  /// @param[out] short_write Whether the size of the data to be sent is equal
  ///             to the size of the data actually sent
  /// @return ssize_t the size of the data that has been sent
  ssize_t FlushTo(IoHandler* io, ConnectionHandler* conn_handler,
                  size_t max_bytes, size_t max_capacity,
                  bool support_pipeline, bool* emptied, bool* short_write);

  /// @brief Append the buffer to be sent to the tail of the list
  BufferAppendStatus Append(NoncontiguousBuffer buffer, IoMessage&& io_msg, size_t max_capacity, int64_t timeout);

  void Stop();

  size_t Size() { return size_; }

 private:
  struct Node {
    std::atomic<Node*> next;
    NoncontiguousBuffer buffer;
    std::atomic<bool> pre_send_flag{false};
    IoMessage io_msg;
  };
  friend struct object_pool::ObjectPoolTraits<Node>;

  void PreSendMessage(bool support_pipeline, ConnectionHandler* conn_handle, Node* current);

 private:
  alignas(hardware_destructive_interference_size) std::atomic<Node*> head_{nullptr};
  alignas(hardware_destructive_interference_size) std::atomic<Node*> tail_{nullptr};
  std::atomic<size_t> size_{0};

  FiberMutex mutex_;
  FiberConditionVariable writable_cv_;
  std::atomic<bool> stop_token_{false};
};

}  // namespace trpc
