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

#include "trpc/stream/fiber_stream_job_scheduler.h"

#include <algorithm>
#include <chrono>

#include "trpc/coroutine/fiber.h"

using namespace std::literals;

namespace trpc::stream {

void FiberStreamJobScheduler::PushRecvMessage(StreamRecvMessage&& msg) {
  bool is_running = true;

  {
    std::scoped_lock lock(stream_mutex_);

    // The stream coroutine has stopped and no longer allows messages to be added.
    if (terminate_) {
      TRPC_FMT_TRACE("stream terminate, forbidden push send message");
      return;
    }

    stream_recv_msgs_.emplace_back(std::move(msg));

    if (!running_) {
      running_ = true;
      is_running = false;
    }
  }

  if (!is_running) {
    bool start_fiber = StartFiberDetached([ref = RefPtr(ref_ptr, this)]() { ref->Run(); });
    TRPC_ASSERT(start_fiber && "FiberStreamJobScheduler PushRecvMessage failed due to fiber create failed");
  }
}

RetCode FiberStreamJobScheduler::PushSendMessage(StreamSendMessage&& msg, bool push_front) {
  bool is_running = true;
  {
    std::scoped_lock lock(stream_mutex_);

    // The stream coroutine has stopped and no longer allows messages to be added.
    if (terminate_) {
      TRPC_FMT_TRACE("stream terminate, forbidden push send message");
      return RetCode::kError;
    }

    if (push_front) {
      stream_send_msgs_.emplace_front(std::move(msg));
    } else {
      stream_send_msgs_.emplace_back(std::move(msg));
    }

    if (!running_) {
      running_ = true;
      is_running = false;
    }
  }

  if (!is_running) {
    bool start_fiber = StartFiberDetached([ref = RefPtr(ref_ptr, this)]() { ref->Run(); });
    TRPC_ASSERT(start_fiber && "FiberStreamJobScheduler PushSendMessage failed due to fiber create failed");
  }

  return RetCode::kSuccess;
}

bool FiberStreamJobScheduler::IsTerminate() const {
  std::scoped_lock lock(stream_mutex_);
  return terminate_;
}

void FiberStreamJobScheduler::Stop() {
  TRPC_FMT_TRACE("Stream stop invoked");
  std::scoped_lock lock(stream_mutex_);
  terminate_ = true;
  // If the stream processing coroutine is not running at this time and the stream needs to be stopped, the necessary
  // operations to stop the stream should be completed here.
  if (!running_) {
    done_ = true;
    stream_cond_.notify_all();
    stream_->GetMutableStreamOptions()->stream_handler->RemoveStream(stream_id_);
  }
}

void FiberStreamJobScheduler::Join() {
  std::unique_lock lk(stream_mutex_);
  // Interval: 1ms.
  while (!stream_cond_.wait_for(lk, 1ms, [this] { return done_; })) { /* do nothing */
  }
}

void FiberStreamJobScheduler::Run() {
  std::unique_lock lk(stream_mutex_, std::defer_lock);
  while (true) {
    lk.lock();
    // When all messages in the queue have been processed, the coroutine can exit normally.
    if (stream_recv_msgs_.empty() && stream_send_msgs_.empty()) {
      // |running_| is only set to true in PushSendMessage/PushRecvMessage when the stream is not terminate.
      // It is only set to false when the is not terminate and the receiving/sending message queue is empty.
      if (terminate_) {
        lk.unlock();
        break;
      }
      running_ = false;
      return;
    }

    auto stream_recv_msgs = std::move(stream_recv_msgs_);
    auto stream_send_msgs = std::move(stream_send_msgs_);

    lk.unlock();

    bool stream_error{false};

    // Processes received message.
    while (!stream_recv_msgs.empty() && !stream_error) {
      StreamRecvMessage recv_msg = std::move(stream_recv_msgs.front());
      stream_recv_msgs.pop_front();
      stream_error = handle_recv_(std::move(recv_msg)) == RetCode::kError;
    }

    // When an error occurs while processing the received stream frame:
    // the exception has already been handled correctly within the stream (sending a Reset control frame to the remote
    // peer other end and pushing a stream processing error message to the business).
    // It is sufficient to exit directly here because the stream is in error and there is no need to send business
    // DATA anymore.
    if (stream_error) {
      Stop();
      break;
    }

    // Processes sending message.
    while (!stream_send_msgs.empty() && !stream_error) {
      StreamSendMessage send_msg = std::move(stream_send_msgs.front());
      stream_send_msgs.pop_front();
      stream_error = handle_send_(std::move(send_msg)) == RetCode::kError;
    }

    if (stream_error) {
      Stop();
      break;
    }
  }
  // The stream has ended, and terminate must be true at this time. There will be no more stream messages coming in.
  // Remove the stream from the StreamHandler.
  stream_->GetMutableStreamOptions()->stream_handler->RemoveStream(stream_id_);
  lk.lock();
  done_ = true;
  stream_cond_.notify_all();
}

}  // namespace trpc::stream
