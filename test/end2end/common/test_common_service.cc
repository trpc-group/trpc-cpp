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

#include "test/end2end/common/test_common_service.h"

namespace trpc::testing {

::trpc::Status TestCommonService::StartService(::trpc::ServerContextPtr context,
                                               const ::trpc::testing::common::ServiceInfo* request,
                                               ::trpc::testing::common::CommonRet* response) {
  bool succ = svr_->StartService(request->service_name());
  response->set_status(succ ? "succ" : "failed");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestCommonService::StopService(::trpc::ServerContextPtr context,
                                              const ::trpc::testing::common::ServiceInfo* request,
                                              ::trpc::testing::common::CommonRet* response) {
  bool succ = svr_->StopService(request->service_name(), true);
  response->set_status(succ ? "succ" : "failed");
  return ::trpc::kSuccStatus;
}

}  // namespace trpc::testing
