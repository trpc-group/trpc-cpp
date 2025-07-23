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

#include "trpc/stream/http/async/stream.h"

namespace trpc::stream {

void HttpAsyncStream::PushRecvMessage(HttpStreamFramePtr&& msg) {
  if (TRPC_UNLIKELY(HandleRecvMessage(std::move(msg)) != RetCode::kSuccess)) {
    Reset(GetStatus());
  }
}

Future<> HttpAsyncStream::AsyncWrite(NoncontiguousBuffer&& msg) {
  // Temporarily use synchronous blocking to send, and will switch to asynchronous sending in the future
  int send_ok = GetMutableStreamOptions()->stream_handler->SendMessage(GetMutableStreamOptions()->context.context,
                                                                       std::move(msg));
  if (send_ok == 0) {
    return MakeReadyFuture<>();
  }

  Status status{send_ok, 0,
                "send stream frame failed, conn_id: " + std::to_string(GetMutableStreamOptions()->connection_id)};
  TRPC_FMT_ERROR(status.ToString());
  OnError(status);

  return MakeExceptionFuture<>(StreamError(GetStatus()));
}

Future<> HttpAsyncStream::PushSendMessage(HttpStreamFramePtr&& msg) {
  NoncontiguousBuffer out;
  if (TRPC_UNLIKELY(HandleSendMessage(std::move(msg), &out) != RetCode::kSuccess)) {
    Reset(GetStatus());
    return MakeExceptionFuture<>(StreamError(GetStatus()));
  }
  // If sending data will cause the flow to be closed, the flow is removed from the connection in advance here. After
  // the server sends a response, the connection can receive new requests immediately.
  if (IsStateTerminate()) {
    Stop();
  }
  // If got empty buffer, there is no need to write to connection.
  // Maybe caused by invoking `WriteDone` at content-length mode.
  if (out.ByteSize() == 0) {
    return MakeReadyFuture<>();
  }
  return AsyncWrite(std::move(out));
}

Future<http::HttpHeader> HttpAsyncStream::AsyncReadHeader(int timeout) {
  pending_header_.val = Promise<http::HttpHeader>();

  auto ft = pending_header_.val.value().GetFuture();

  // header data is ready
  if (header_.has_value()) {
    NotifyPending(&pending_header_, std::move(header_));
    header_.reset();
  } else if (trailer_.has_value()) {
    NotifyPending(&pending_header_, std::move(trailer_));
    trailer_.reset();
  }

  if (timeout == std::numeric_limits<int>::max() || ft.IsReady() || ft.IsFailed()) {
    return ft;
  }

  // create a task that waits for the header to be ready
  CreatePendingTimer(&pending_header_, timeout);

  return ft;
}

Future<NoncontiguousBuffer> HttpAsyncStream::AsyncReadChunk(int timeout) {
  // it can read only in chunked mode
  if (read_mode_ != DataMode::kChunked) {
    Status status{TRPC_STREAM_UNKNOWN_ERR, 0, "Can't read no chunk data"};
    TRPC_FMT_ERROR(status.ErrorMessage());
    return MakeExceptionFuture<NoncontiguousBuffer>(StreamError(status));
  }
  return AsyncReadInner(ReadOperation::kReadChunk, 0, timeout);
}

Future<NoncontiguousBuffer> HttpAsyncStream::AsyncReadAtMost(uint64_t len, int timeout) {
  return AsyncReadInner(ReadOperation::kReadAtMost, len, timeout);
}

Future<NoncontiguousBuffer> HttpAsyncStream::AsyncReadExactly(uint64_t len, int timeout) {
  return AsyncReadInner(ReadOperation::kReadExactly, len, timeout);
}

Future<NoncontiguousBuffer> HttpAsyncStream::AsyncReadInner(ReadOperation op, uint64_t len, int timeout) {
  if (read_mode_ == DataMode::kNoData) {
    // can not read data when content-length equal to 0
    return MakeReadyFuture<NoncontiguousBuffer>(NoncontiguousBuffer{});
  }
  PendingData pending(op, len);
  pending.data.val = Promise<NoncontiguousBuffer>();

  auto ft = pending.data.val.value().GetFuture();
  // push the read request to pending queue
  read_queue_.push(std::move(pending));
  // read if the data is ready
  NotifyPendingDataQueue();

  if (timeout == std::numeric_limits<int>::max() || ft.IsReady() || ft.IsFailed()) {
    return ft;
  }

  // create a task that waits for the data to be ready
  CreatePendingTimer(&(read_queue_.back().data), timeout);

  return ft;
}

void HttpAsyncStream::OnHeader(http::HttpHeader* header) {
  if (!pending_header_.val) {  // there are no calls waiting headers
    if (read_state_ == State::kHalfClosed) {
      trailer_ = std::make_optional<http::HttpHeader>(std::move(*header));
      return;
    }
    header_ = std::make_optional<http::HttpHeader>(std::move(*header));
    return;
  }
  // there is a call waiting for the header, notify it
  NotifyPending(&pending_header_, std::make_optional<http::HttpHeader>(std::move(*header)));
}

void HttpAsyncStream::OnData(NoncontiguousBuffer* data) {
  read_left_bytes_ += data->ByteSize();
  if (has_full_message_ && read_mode_ == DataMode::kContentLength) {
    check_data_ = std::move(*data);
  } else {
    data_block_.push_back(std::move(*data));
  }
  NotifyPendingDataQueue();
}

void HttpAsyncStream::OnEof() {
  read_eof_ = true;
  NotifyPendingDataQueue();
}

void HttpAsyncStream::OnError(Status status) {
  status_ = status;

  if (pending_header_.val) {
    if (header_.has_value()) {
      NotifyPending(&pending_header_, std::move(header_));
    } else {
      NotifyPending(&pending_header_, std::move(trailer_));
    }
    header_.reset();
    trailer_.reset();
  }
  NotifyPendingDataQueue();
}

void HttpAsyncStream::NotifyPendingDataQueue() {
  while (!read_queue_.empty()) {
    PendingData& pd = read_queue_.front();

    if (!pd.data.val.has_value()) {
      // clean up the timeout pending data
      read_queue_.pop();
      continue;
    }

    auto& prom = pd.data.val.value();

    if (read_left_bytes_ == 0) {              // no data to read
      if (!NotifyPendingCheck(&(pd.data))) {  // there is an exception in the flow
        read_queue_.pop();
        continue;
      }
      if (read_eof_) {  // reach eof
        prom.SetValue(NoncontiguousBuffer());
        read_queue_.pop();
        continue;
      }
      // No data and not eof, no need to read. Need to wait for data to arrive in the network
      return;
    }

    // read the chunked block if there has chunked block
    if (pd.op == ReadOperation::kReadChunk && data_block_.size() > 0) {
      read_left_bytes_ -= data_block_.front().ByteSize();
      prom.SetValue(std::move(data_block_.front()));
      PendingDone(&(pd.data));
      data_block_.pop_front();

      read_queue_.pop();
      continue;
    }

    std::size_t want_read_bytes;
    if (pd.op == ReadOperation::kReadAtMost) {
      want_read_bytes = std::min(pd.except_bytes, read_left_bytes_);
    } else if (pd.op == ReadOperation::kReadExactly) {
      want_read_bytes = read_eof_ ? std::min(pd.except_bytes, read_left_bytes_) : pd.except_bytes;
    } else {
      TRPC_ASSERT(false && "Unreachable");
    }
    if (read_left_bytes_ < want_read_bytes) {
      // there are no enough data to read
      return;
    }

    read_left_bytes_ -= want_read_bytes;
    NoncontiguousBufferBuilder builder;
    if (has_full_message_ && read_mode_ == DataMode::kContentLength) {
      prom.SetValue(check_data_.Cut(std::min(check_data_.ByteSize(), want_read_bytes)));
    } else {
      while (want_read_bytes > 0) {
        std::size_t block_size = data_block_.front().ByteSize();
        std::size_t cut_bytes = std::min(block_size, want_read_bytes);
        builder.Append(data_block_.front().Cut(cut_bytes));
        if (data_block_.front().Empty()) {
          data_block_.pop_front();
        }
        want_read_bytes -= cut_bytes;
      }
      prom.SetValue(builder.DestructiveGet());
    }
    PendingDone(&(pd.data));

    read_queue_.pop();
  }
}

}  // namespace trpc::stream
