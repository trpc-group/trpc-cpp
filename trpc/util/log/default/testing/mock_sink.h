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

#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/version.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class MockSink : public trpc::RefCounted<MockSink> {
 public:
  class Sink : public spdlog::sinks::sink {
   public:
    MOCK_METHOD(void, log, (const spdlog::details::log_msg& msg), (override));
    MOCK_METHOD(void, flush, (), (override));
    MOCK_METHOD(void, set_pattern, (const std::string& pattern), (override));
    MOCK_METHOD(void, set_formatter, (std::unique_ptr<spdlog::formatter> sink_formatter), (override));
  };
  MOCK_METHOD(spdlog::sink_ptr, SpdSink, (), (const));
};

using MockSinkPtr = trpc::RefPtr<MockSink>;

}  // namespace trpc
