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

#include "test/end2end/common/test_common.trpc.pb.h"

#include "trpc/common/trpc_app.h"

namespace trpc::testing {

/// @brief Used to a testing scenario in which the server will serve a service with different config files.
/// Services can be dynamically started to save number of listening ports. The server should registery a bunch of
/// services with different config files including this service.
class TestCommonService : public ::trpc::testing::common::TestCommon {
 public:
  explicit TestCommonService(::trpc::TrpcApp* svr) : svr_(svr) {}

  /// @brief Dynamic start a service. You can just do rpc invoke at client side.
  /// @param request ServiceInfo has string filed service_name to indicate which service need to start.
  /// @param response CommonRet has string filed(`succ`, `failed`) status to indicate operation result.
  ::trpc::Status StartService(::trpc::ServerContextPtr context, const ::trpc::testing::common::ServiceInfo* request,
                              ::trpc::testing::common::CommonRet* response) override;

  /// @brief Dynamic stop a service. You can just do rpc invoke at client side.
  /// @param request ServiceInfo has string filed service_name to indicate which service need to start.
  /// @param response CommonRet has string filed(`succ`, `failed`) status to indicate operation result.
  ::trpc::Status StopService(::trpc::ServerContextPtr context, const ::trpc::testing::common::ServiceInfo* request,
                             ::trpc::testing::common::CommonRet* response) override;

 private:
  ::trpc::TrpcApp* svr_{nullptr};
};

}  // namespace trpc::testing
