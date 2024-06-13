// Copyright (c) 2024, Tencent Inc.
// All rights reserved.

#pragma once

#include <atomic>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/protocol.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/overload_control/server_overload_controller.h"
#include "trpc/server/service.h"
#include "trpc/server/testing/service_adapter_testing.h"

namespace trpc::overload_control {
namespace testing {

// Mock protocol, only allowed to be used at overload control module.
class MockProtocol : public Protocol {
 public:
  MOCK_METHOD1(ZeroCopyDecode, bool(NoncontiguousBuffer&));
  MOCK_METHOD1(ZeroCopyEncode, bool(NoncontiguousBuffer&));
  MOCK_METHOD1(SetCallType, void(RpcCallType));
  MOCK_METHOD0(GetCallType, RpcCallType());
};

using MockProtocolPtr = std::shared_ptr<MockProtocol>;

// Get filter object by filter point and filter name.
inline MessageServerFilterPtr GetGlobalServerFilterByName(FilterPoint type, const std::string& name) {
  const std::deque<MessageServerFilterPtr>& filters = FilterManager::GetInstance()->GetMessageServerGlobalFilter(type);
  for (auto& filter : filters) {
    if (!filter->Name().compare(name)) {
      return filter;
    }
  }
  return nullptr;
}

class MockServerOverloadController : public ServerOverloadController {
 public:
  MOCK_METHOD0(Name, std::string());
  MOCK_METHOD0(Init, bool());
  MOCK_METHOD1(BeforeSchedule, bool(const ServerContextPtr&));
  MOCK_METHOD1(AfterSchedule, bool(const ServerContextPtr&));
};

}  // namespace testing
}  // namespace trpc::overload_control
