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

#pragma once

#include <any>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/stream/trpc/trpc_stream.h"
#include "trpc/util/function.h"

namespace trpc::stream {

/// @brief Stream handler of tRPC stream.
class TrpcStreamHandler : public StreamHandler {
 public:
  explicit TrpcStreamHandler(StreamOptions&& options) : options_{std::move(options)} {}

  ~TrpcStreamHandler() = default;

  bool Init() override { return true; }

  void Stop() override;

  void Join() override;

  int RemoveStream(uint32_t stream_id) override;

  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override;

  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override;

  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  int EncodeTransportMessage(IoMessage* msg) override { return 0; }

 protected:
  virtual int GetNetworkErrorCode() { return -1; }

  StreamRecvMessage CreateResetMessage(uint32_t stream_id, const Status& status);

  template <class T>
  T CriticalSection(Function<T()>&& cb) const {
    if (options_.fiber_mode) {
      std::unique_lock<FiberMutex> _(mutex_);
      return cb();
    } else {
      return cb();
    }
  }

 private:
  int RemoveStreamInner(uint32_t stream_id);

 protected:
  StreamOptions options_;

  std::unordered_map<uint32_t, TrpcStreamPtr> streams_;

  mutable FiberMutex mutex_;

  bool conn_closed_{false};
};

}  // namespace trpc::stream
