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

#include "trpc/stream/common_stream.h"

#include <optional>

#include "trpc/common/future/future_utility.h"

namespace trpc::stream {

std::string_view CommonStream::StreamStateToString(State state) {
  switch (state) {
    case State::kClosed:
      return "closed";
    case State::kIdle:
      return "idle";
    case State::kInit:
      return "init";
    case State::kOpen:
      return "open";
    case State::kLocalClosed:
      return "local_closed";
    case State::kRemoteClosed:
      return "remote_closed";
    default:
      TRPC_ASSERT(false && "Not predefined State in CommonStream");
  }
}

std::string_view CommonStream::StreamActionToString(Action action) {
  switch (action) {
    case Action::kSendInit:
      return "send init";
    case Action::kSendData:
      return "send data";
    case Action::kSendFeedback:
      return "send feedback";
    case Action::kSendClose:
      return "send close";
    case Action::kHandleInit:
      return "handle init";
    case Action::kHandleData:
      return "handle data";
    case Action::kHandleFeedback:
      return "handle feedback";
    case Action::kHandleClose:
      return "handle close";
    default:
      TRPC_ASSERT(false && "Not predefined Action in CommonStream");
  }
}

std::string_view CommonStream::StreamMessageCategoryToString(StreamMessageCategory c) {
  switch (c) {
    case StreamMessageCategory::kStreamData:
      return "stream_data";
    case StreamMessageCategory::kStreamInit:
      return "stream_init";
    case StreamMessageCategory::kStreamFeedback:
      return "stream_feedback";
    case StreamMessageCategory::kStreamClose:
      return "stream_close";
    default:
      return "stream_unknown";
  }
}

void CommonStream::PushRecvMessage(StreamRecvMessage&& msg) {
  TRPC_FMT_TRACE("stream, push recv message, stream id: {}, message category: {}", GetId(),
                 StreamMessageCategoryToString(StreamMessageCategory{msg.metadata.stream_frame_type}));

  if (fiber_entity_) {
    return fiber_entity_->PushRecvMessage(std::move(msg));
  }

  if (HandleRecvMessage(std::move(msg)) != RetCode::kSuccess) {
    Stop();
  }
}

RetCode CommonStream::PushSendMessage(StreamSendMessage&& msg, bool push_front) {
  TRPC_FMT_TRACE("stream, push send message, stream id: {}, message category: {}", GetId(),
                 StreamMessageCategoryToString(msg.category));

  if (fiber_entity_) {
    return fiber_entity_->PushSendMessage(std::move(msg), push_front);
  }

  auto ret = HandleSendMessage(std::move(msg));
  if (ret != RetCode::kSuccess) {
    Stop();
  }

  return ret;
}

void CommonStream::Stop() {
  if (fiber_entity_) {
    return fiber_entity_->Stop();
  } else if (!reader_writer_options_.done.IsResolved()) {
    reader_writer_options_.done.SetValue();
  }

  if (reader_writer_options_.pending_timer_id != iotimer::InvalidID) {
    TRPC_CHECK(iotimer::Cancel(reader_writer_options_.pending_timer_id));
  }

  TRPC_FMT_WARN_IF(options_.stream_handler->RemoveStream(GetId()) != 0, "Stream already stopped?");
}

void CommonStream::Join() {
  if (fiber_entity_) return fiber_entity_->Join();
}

CommonStream::CommonStream(StreamOptions&& options) : options_(std::move(options)) {
  if (options_.fiber_mode) {
    fiber_entity_ = MakeRefCounted<FiberStreamJobScheduler>(
        GetId(), this, [this](StreamRecvMessage&& msg) { return HandleRecvMessage(std::move(msg)); },
        [this](StreamSendMessage&& msg) { return HandleSendMessage(std::move(msg)); });
  }
  reader_writer_options_.simple_state = ReaderWriterOptions::NotReady;
  StreamVarHelper::GetInstance()->IncrementActiveStreamCount();
}

CommonStream::~CommonStream() { StreamVarHelper::GetInstance()->DecrementActiveStreamCount(); }

Status CommonStream::GetStatus() const {
  return CriticalSection<Status>([this]() { return reader_writer_options_.status; });
}

Status CommonStream::Read(NoncontiguousBuffer* msg, int timeout) {
  // Read message from stream message reader queue, wait for specified timeout if the queue is empty.
  // If timeout, return timeout error code.
  std::unique_lock lock(reader_writer_options_.mutex);
  bool no_timeout = reader_writer_options_.cond.wait_for(lock, GetTimeout(timeout), [this]() {
    return !reader_writer_options_.reader_msgs.empty() ||
           reader_writer_options_.simple_state == ReaderWriterOptions::Error;
  });

  if (!no_timeout) {
    return Status(GetReadTimeoutErrorCode(), 0, "read message timeout");
  }

  // If there is a response message in the queue, read one.
  // Read the messages that are already in place first, otherwise reading again will result in an error.
  if (!reader_writer_options_.reader_msgs.empty()) {
    auto& first_msg = reader_writer_options_.reader_msgs.front();
    if (!first_msg) {
      if (reader_writer_options_.simple_state == ReaderWriterOptions::Error) {
        return reader_writer_options_.status;
      }
      return GetEofStatus();
    }
    *msg = std::move(first_msg.value());
    reader_writer_options_.reader_msgs.pop();
    return reader_writer_options_.status;
  } else if (reader_writer_options_.simple_state == ReaderWriterOptions::Error) {
    return reader_writer_options_.status;
  }
  // unreachable
  return kUnknownErrorStatus;
}

Future<std::optional<NoncontiguousBuffer>> CommonStream::AsyncRead(int timeout) {
  reader_writer_options_.pending = Promise<std::optional<NoncontiguousBuffer>>();
  auto ft = reader_writer_options_.pending.value().GetFuture();
  NotifyPending();

  if (timeout == -1 || ft.IsReady() || ft.IsFailed()) return ft;

  TRPC_CHECK_EQ(reader_writer_options_.pending_timer_id, iotimer::InvalidID);
  reader_writer_options_.pending_timer_id = iotimer::Create(timeout, 0, [this]() {
    if (!reader_writer_options_.pending) return;
    reader_writer_options_.pending.value().SetException(
        Timeout(Status(GetReadTimeoutErrorCode(), 0, "read message timeout")));
    PendingDone();
  });
  TRPC_CHECK_NE(reader_writer_options_.pending_timer_id, iotimer::InvalidID);

  return ft;
}

Status CommonStream::Write(NoncontiguousBuffer&& msg) {
  auto status = GetStatus();
  if (!status.OK()) {
    return status;
  }

  PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamData, .data_provider = std::move(msg)});
  return GetStatus();
}

Future<> CommonStream::AsyncWrite(NoncontiguousBuffer&& msg) {
  if (GetStatus().OK()) {
    if (PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamData,
                                          .data_provider = std::move(msg)}) == RetCode::kSuccess)
      return MakeReadyFuture<>();
  }

  return MakeExceptionFuture<>(StreamError(GetStatus()));
}

Status CommonStream::WriteDone() { return GetStatus(); }

Status CommonStream::Start() {
  std::unique_lock lock(reader_writer_options_.mutex);
  bool no_timeout = reader_writer_options_.cond.wait_for(lock, GetTimeout(options_.stream_read_timeout), [this]() {
    return reader_writer_options_.simple_state != ReaderWriterOptions::NotReady;
  });
  lock.unlock();

  if (no_timeout) {
    return GetStatus();
  } else {
    Status status{GetReadTimeoutErrorCode(), 0, "wait for stream to be ready timeout"};
    TRPC_LOG_ERROR(status.ToString());
    // Set the failed status, the business logic can detect this error.
    reader_writer_options_.status = status;
    // Send a timeout failure error to the remote peer.
    Reset(status);
    Stop();
    return status;
  }
}

Future<> CommonStream::AsyncStart() {
  // FIXME: timeout
  return reader_writer_options_.ready.GetFuture();
}

Future<> CommonStream::AsyncFinish() { return reader_writer_options_.done.GetFuture(); }

RetCode CommonStream::HandleSendMessage(StreamSendMessage&& msg) {
  TRPC_FMT_TRACE("stream, handle send messag, stream id: {}, message category: {}", GetId(),
                 StreamMessageCategoryToString(msg.category));

  RetCode ret{RetCode::kSuccess};

  switch (msg.category) {
    case StreamMessageCategory::kStreamData: {
      ret = SendData(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamInit: {
      ret = SendInit(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamFeedback: {
      ret = SendFeedback(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamClose: {
      ret = SendClose(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamReset: {
      ret = SendReset(std::move(msg));
      break;
    }
    default: {
      Status status{GetEncodeErrorCode(), 0, "unsupported trpc stream frame type."};
      OnError(status);
      Reset(status);
      TRPC_LOG_ERROR(status.ErrorMessage());
    }
  }

  return ret;
}

bool CommonStream::Send(NoncontiguousBuffer&& buffer) {
  int send_ok = options_.stream_handler->SendMessage(options_.context.context, std::move(buffer));
  if (send_ok == 0) {
    return true;
  }

  Status status{send_ok, 0,
                "send stream frame failed, stream_id:" + std::to_string(GetId()) +
                    ", conn_id: " + std::to_string(options_.connection_id)};
  TRPC_LOG_ERROR(status.ToString());
  OnError(status);

  return false;
}

RetCode CommonStream::HandleRecvMessage(StreamRecvMessage&& msg) {
  TRPC_FMT_TRACE("stream, handle recv message, stream id: {}, message category: {}", GetId(),
                 StreamMessageCategoryToString(StreamMessageCategory{msg.metadata.stream_frame_type}));

  RetCode ret{RetCode::kSuccess};

  switch (static_cast<StreamMessageCategory>(msg.metadata.stream_frame_type)) {
    case StreamMessageCategory::kStreamData: {
      ret = HandleData(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamInit: {
      ret = HandleInit(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamClose: {
      ret = HandleClose(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamFeedback: {
      ret = HandleFeedback(std::move(msg));
      break;
    }
    case StreamMessageCategory::kStreamReset: {
      ret = HandleReset(std::move(msg));
      break;
    }
    default: {
      Status status{GetDecodeErrorCode(), 0, "unsupported trpc stream frame type."};
      OnError(status);
      Reset(status);
      TRPC_LOG_ERROR(status.ErrorMessage());
    }
  }

  return ret;
}

void CommonStream::OnReady() {
  TRPC_FMT_TRACE("stream, on ready, stream id: {}", GetId());

  bool already = CriticalSection<bool>([this]() {
    if (reader_writer_options_.simple_state == ReaderWriterOptions::Ready) return true;
    reader_writer_options_.simple_state = ReaderWriterOptions::Ready;
    return false;
  });

  if (already) {
    TRPC_FMT_DEBUG("Unexpected ready");
    return;
  }

  if (options_.fiber_mode)
    reader_writer_options_.cond.notify_all();
  else
    reader_writer_options_.ready.SetValue();
}

void CommonStream::OnData(NoncontiguousBuffer&& msg) {
  // Push the message to the end of the queue.
  CriticalSection<void>([this, msg = std::move(msg)]() { reader_writer_options_.reader_msgs.emplace(std::move(msg)); });

  TRPC_FMT_DEBUG("stream {}, push back, reader message queue size: {}", GetMutableStreamOptions()->server_mode,
                 reader_writer_options_.reader_msgs.size());

  if (options_.fiber_mode)
    reader_writer_options_.cond.notify_all();
  else
    NotifyPending();
}

void CommonStream::OnFinish(Status status) {
  // Push the EOF to the end of the queue.
  bool already = CriticalSection<bool>([this, status]() {
    if (reader_writer_options_.simple_state == ReaderWriterOptions::Finished) return true;
    reader_writer_options_.reader_msgs.emplace(std::nullopt);
    reader_writer_options_.status = status;
    reader_writer_options_.simple_state = ReaderWriterOptions::Finished;
    return false;
  });

  if (already) {
    TRPC_FMT_DEBUG("Unexpected EOF");
    return;
  }

  if (options_.fiber_mode) {
    reader_writer_options_.cond.notify_all();
  } else {
    reader_writer_options_.done.SetValue();
    NotifyPending();
  }
}

void CommonStream::OnError(Status status) {
  TRPC_CHECK(!status.OK());
  bool already = CriticalSection<bool>([this, status]() {
    if (reader_writer_options_.simple_state == ReaderWriterOptions::Error) return true;
    reader_writer_options_.status = status;
    reader_writer_options_.simple_state = ReaderWriterOptions::Error;
    return false;
  });

  if (already) {
    TRPC_FMT_DEBUG("Unexpected error");
    return;
  }

  if (options_.fiber_mode) {
    reader_writer_options_.cond.notify_all();
  } else {
    reader_writer_options_.ready.SetException(StreamError(std::move(status)));
    reader_writer_options_.done.SetValue();
    NotifyPending();
  }

  if (GetStreamVar()) {
    GetStreamVar()->AddRpcCallFailureCount(1);
  }
}

void CommonStream::NotifyPending() {
  if (!reader_writer_options_.pending) return;
  auto& pr = reader_writer_options_.pending.value();

  auto left = reader_writer_options_.reader_msgs.size();

  // Ready message.
  if (left > 0) {
    auto& first_msg = reader_writer_options_.reader_msgs.front();
    if (first_msg) {
      // data
      pr.SetValue(std::move(first_msg));
      reader_writer_options_.reader_msgs.pop();
    } else {
      // eof
      if (reader_writer_options_.simple_state == ReaderWriterOptions::Error) {
        TRPC_FMT_WARN_IF(left > 0, "stream {} error with {} msg/EOF unread.", GetId(), left);
        pr.SetException(StreamError(reader_writer_options_.status));
        PendingDone();
        return;
      }
      pr.SetValue(first_msg);
    }
    PendingDone();
    TRPC_FMT_DEBUG("stream {} read one message, left {} msgs/EOF.", GetId(), left - 1);
    return;
  }
  // Error message.
  if (reader_writer_options_.simple_state == ReaderWriterOptions::Error) {
    TRPC_FMT_WARN_IF(left > 0, "stream {} error with {} msg/EOF unread.", GetId(), left);
    pr.SetException(StreamError(reader_writer_options_.status));
    PendingDone();
    return;
  }
}

void CommonStream::PendingDone() {
  reader_writer_options_.pending.reset();
  if (reader_writer_options_.pending_timer_id != iotimer::InvalidID) {
    TRPC_CHECK(iotimer::Cancel(reader_writer_options_.pending_timer_id));
  }
  reader_writer_options_.pending_timer_id = iotimer::InvalidID;
}

}  // namespace trpc::stream
