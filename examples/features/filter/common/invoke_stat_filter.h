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

#include <atomic>

#include "trpc/common/logging/trpc_logging.h"
#include "trpc/filter/filter.h"

namespace examples::filter {

class ServerInvokeStatFilter : public ::trpc::MessageServerFilter {
 public:
  std::string Name() override { return "server_invoke_stat"; }

  std::vector<::trpc::FilterPoint> GetFilterPoint() override {
    std::vector<::trpc::FilterPoint> points = {::trpc::FilterPoint::SERVER_PRE_RPC_INVOKE,
                                               ::trpc::FilterPoint::SERVER_POST_RPC_INVOKE};
    return points;
  }

  void operator()(::trpc::FilterStatus& status, ::trpc::FilterPoint point,
                  const ::trpc::ServerContextPtr& context) override {
    if (point == ::trpc::FilterPoint::SERVER_POST_RPC_INVOKE) {
      if (context->GetStatus().OK()) {
        success_count_++;
      } else {
        fail_count_++;
      }
      TRPC_FMT_INFO("Invoke stat: success count = {}, fail count = {}", success_count_, fail_count_);
    }
  }

 private:
  std::atomic<uint64_t> fail_count_{0};
  std::atomic<uint64_t> success_count_{0};
};

class ClientInvokeStatFilter : public ::trpc::MessageClientFilter {
 public:
  std::string Name() override { return "client_invoke_stat"; }

  std::vector<::trpc::FilterPoint> GetFilterPoint() override {
    std::vector<::trpc::FilterPoint> points = {::trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE,
                                               ::trpc::FilterPoint::CLIENT_POST_RPC_INVOKE};
    return points;
  }

  void operator()(::trpc::FilterStatus& status, ::trpc::FilterPoint point,
                  const ::trpc::ClientContextPtr& context) override {
    if (point == ::trpc::FilterPoint::CLIENT_POST_RPC_INVOKE) {
      if (context->GetStatus().OK()) {
        success_count_++;
      } else {
        fail_count_++;
      }
      TRPC_FMT_INFO("Invoke stat: success count = {}, fail count = {}", success_count_, fail_count_);
    }
  }

 private:
  std::atomic<uint64_t> fail_count_{0};
  std::atomic<uint64_t> success_count_{0};
};

}  // namespace examples::filter
