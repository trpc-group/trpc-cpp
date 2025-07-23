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

#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/testing/mock_connection_testing.h"
#include "trpc/server/testing/stream_transport_testing.h"
#include "trpc/stream/stream_message.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::testing {

uint32_t NextStreamId();

class TestTrpcClientStreamProvider : public trpc::stream::StreamReaderWriterProvider {
 public:
  struct Options {
    uint32_t stream_id;
    std::string func;
    std::string caller;
    std::string callee;
    std::shared_ptr<TestStreamTransport> transport{nullptr};
    bool fiber_mode{true};
    trpc::NoncontiguousBuffer reply_msg;
  };

 public:
  TestTrpcClientStreamProvider(const Options& options);

  trpc::Status Start() override {
    if (SendAndRecvInit()) return trpc::kSuccStatus;
    return kUnknownErrorStatus;
  }

  trpc::Status Read(NoncontiguousBuffer* msg, int timeout = -1) override;

  trpc::Status Write(NoncontiguousBuffer&& msg) override;

  trpc::Status Finish() override { return kSuccStatus; }

  trpc::Status GetStatus() const override { return kSuccStatus; }

  trpc::Status WriteDone() override;

 private:
  bool SendAndRecvInit();
  trpc::Status HandleClose(NoncontiguousBuffer& msg);
  bool Recv(trpc::stream::ProtocolMessageMetadata& meta, NoncontiguousBuffer& data);
  int Check(trpc::NoncontiguousBuffer& in, std::deque<std::any>& out);
  bool Pick(const std::any& msg, trpc::stream::ProtocolMessageMetadata& metadata);

 private:
  Options options_;
  trpc::NoncontiguousBuffer recv_buff_;
  std::deque<std::any> recv_msgs_;
  trpc::ConnectionPtr conn_;
};

}  // namespace trpc::testing
