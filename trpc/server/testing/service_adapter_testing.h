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

#include <deque>
#include <iostream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/server/server_context.h"
#include "trpc/server/service.h"
#include "trpc/server/service_adapter.h"
#include "trpc/transport/common/transport_message.h"
#include "trpc/util/time.h"

namespace trpc::testing {

/// @brief Create service adapter option for further service adapter creation.
[[maybe_unused]] static trpc::ServiceAdapterOption CreateServiceAdapterOption() {
  trpc::ServiceAdapterOption option;
  option.service_name = "trpc.test.helloworld.Greeter";
  option.network = "tcp";
  option.ip = "0.0.0.0";
  option.is_ipv6 = false;
  option.port = 12345;
  option.protocol = "trpc";
  option.timeout = 2000;
  option.max_conn_num = 10;
  option.max_packet_size = 10000000;

  return option;
}

/// @brief Related service and adapter.
/// @note service must not be null.
[[maybe_unused]] static void FillServiceAdapter(trpc::ServiceAdapter* adapter,
                                                const std::string& service_name,
                                                const trpc::ServicePtr& service) {
  service->SetName(service_name);
  adapter->SetService(service);
  service->SetAdapter(adapter);
}

}  // namespace trpc::testing
