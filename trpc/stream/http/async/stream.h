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

#pragma once

#include "trpc/stream/http/common/stream.h"

#include "trpc/common/async_timer.h"

namespace trpc::stream {

/// @brief The asynchronous stream of http. It implements asynchronous reading logic for headers and body, as well as
///        sending data.
class HttpAsyncStream : public HttpCommonStream {
 public:
  explicit HttpAsyncStream(StreamOptions&& options) : HttpCommonStream(std::move(options)) {}

  /// @brief Pushes the received stream protocol frame message.
  void PushRecvMessage(HttpStreamFramePtr&& msg) override;

  /// @brief Asynchronously writes a message to the stream.
  Future<> AsyncWrite(NoncontiguousBuffer&& msg) override;

  /// @brief Pushed the stream frame need to be send
  Future<> PushSendMessage(HttpStreamFramePtr&& msg);

  /// @brief Reads the header asynchronously.
  /// @param timeout time to wait for the header to be ready
  Future<http::HttpHeader> AsyncReadHeader(int timeout = std::numeric_limits<int>::max());

  /// @brief Reads a chunk in chunked mode asynchronously, note that reading in non-chunked mode will fail
  /// @param timeout time to wait for the header to be ready
  Future<NoncontiguousBuffer> AsyncReadChunk(int timeout = std::numeric_limits<int>::max());

  /// @brief Reads at most len data asynchronously.
  /// @param len max size to read
  /// @param timeout time to wait for the header to be ready
  /// @note If the size of the data obtained from the network is smaller than len, it will return size length data.
  ///       If the size of the data obtained from the network is greater than len, it will return len length data.
  ///       An empty buffer means that the end has been read
  ///       Usage scenario 1: Limits the maximum length of each read When the memory is limited.
  ///       Usage scenario 2: Gets part of data in time and send it downstream on route server.
  Future<NoncontiguousBuffer> AsyncReadAtMost(uint64_t len, int timeout = std::numeric_limits<int>::max());

  /// @brief Reads data with a fixed length. If eof is read, it will return as much data as there is in the network
  /// @param len size to read
  /// @param timeout time to wait for the header to be ready
  /// @note If the read buffer size is less than the required length, it means that eof has been read.
  ///       Usage scenario 1: The requested data is compressed by a fixed size, and needs to be read and decompressed by
  ///       a fixed size.
  Future<NoncontiguousBuffer> AsyncReadExactly(uint64_t len, int timeout = std::numeric_limits<int>::max());

 protected:
  template <class T>
  struct PendingVal {
    std::optional<Promise<T>> val;
    uint64_t timer_id{iotimer::InvalidID};
  };

  /// @brief The way to read
  enum ReadOperation {
    kReadChunk,
    kReadAtMost,
    kReadExactly,
  };

  /// @brief The pending struct for data reading request
  struct PendingData {
    PendingVal<NoncontiguousBuffer> data;
    uint64_t except_bytes{0};
    ReadOperation op;

    explicit PendingData(ReadOperation read_op, uint64_t bytes = 0) : except_bytes(bytes), op(read_op) {}
  };

 protected:
  void OnHeader(http::HttpHeader* header) override;

  void OnData(NoncontiguousBuffer* data) override;

  void OnEof() override;

  void OnError(Status status) override;

  /// @brief Resets the PendingVal
  template <class T>
  void PendingDone(PendingVal<T>* pending);

  /// @brief Creates a scheduled waiting task
  template <class T>
  void CreatePendingTimer(PendingVal<T>* pending, int timeout);

  /// @brief Checks the pending state
  template <class T>
  bool NotifyPendingCheck(PendingVal<T>* pending);

  /// @brief Notifies that the data is reached
  template <class T>
  void NotifyPending(PendingVal<T>* pending, std::optional<T>&& data);

  void NotifyPendingDataQueue();

  Future<NoncontiguousBuffer> AsyncReadInner(ReadOperation op, uint64_t len, int timeout);

 protected:
  /// @brief Used to store asynchronous data request
  std::queue<PendingData> read_queue_;
  /// @brief Used to read data asynchronously. If it is chunked mode, each element is a chunk block; if it is
  ///        content-length mode, each element is all the data from each check
  std::list<NoncontiguousBuffer> data_block_;
  /// @brief It is used to optimize the performance of checking out all data at one time in length mode, avoiding the
  ///        performance overhead caused by using list (can optimize the scenario of unary rpc)
  NoncontiguousBuffer check_data_;
  /// @brief The number of bytes still in the stream that have not been read
  std::size_t read_left_bytes_{0};

  /// @brief The flag indicating whether has read eof
  bool read_eof_{false};

  /// @brief Used to store header
  std::optional<http::HttpHeader> header_;
  /// @brief Used to store trailer
  std::optional<http::HttpHeader> trailer_;
  /// @brief Used to store asynchronous header request
  PendingVal<http::HttpHeader> pending_header_;
};
using HttpAsyncStreamPtr = RefPtr<HttpAsyncStream>;


template <class T>
void HttpAsyncStream::PendingDone(PendingVal<T>* pending) {
  pending->val.reset();
  if (pending->timer_id != iotimer::InvalidID) {
    TRPC_CHECK(iotimer::Cancel(pending->timer_id));
  }
  pending->timer_id = iotimer::InvalidID;
}

template <class T>
void HttpAsyncStream::CreatePendingTimer(PendingVal<T>* pending, int timeout) {
  TRPC_CHECK_EQ(pending->timer_id, iotimer::InvalidID);
  pending->timer_id = iotimer::Create(timeout, 0, [this, pending]() {
    if (!pending->val) {
      return;
    }
    pending->val.value().SetException(Timeout(Status(GetReadTimeoutErrorCode(), 0, "read message timeout")));
    PendingDone(pending);
  });
}

template <class T>
bool HttpAsyncStream::NotifyPendingCheck(PendingVal<T>* pending) {
  if (!pending->val) {
    return false;
  }
  if (!status_.OK()) {
    auto& prom = pending->val.value();
    prom.SetException(StreamError(status_));
    PendingDone(pending);
    return false;
  }
  return true;
}

template <class T>
void HttpAsyncStream::NotifyPending(PendingVal<T>* pending, std::optional<T>&& data) {
  if (!NotifyPendingCheck(pending)) {
    return;
  }
  auto& prom = pending->val.value();
  if (data.has_value()) {
    prom.SetValue(std::move(*data));
  } else {
    Status status{TRPC_STREAM_UNKNOWN_ERR, 0, "Read nothing"};
    TRPC_FMT_ERROR(status.ErrorMessage());
    prom.SetException(StreamError(status));
  }
  PendingDone(pending);
}

}  // namespace trpc::stream
