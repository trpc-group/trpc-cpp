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

#include <string>

#include "gmock/gmock.h"

#include "trpc/codec/grpc/http2/session.h"

namespace trpc::testing {
class MockHttp2Session : public http2::Session {
 public:
  explicit MockHttp2Session(http2::SessionOptions&& options) : http2::Session(std::move(options)) {}
  ~MockHttp2Session() override = default;

  bool Init() override { return true; }
  MOCK_METHOD(int, OnMessage, (NoncontiguousBuffer*, std::deque<std::any>*), (override));
  MOCK_METHOD(int, SubmitRequest, (const http2::RequestPtr&), (override));
  MOCK_METHOD(int, SubmitResponse, (const http2::ResponsePtr&), (override));
  MOCK_METHOD(int, SignalRead, (NoncontiguousBuffer*), (override));
  MOCK_METHOD(int, SignalWrite, (NoncontiguousBuffer*), (override));

  MOCK_METHOD(int, SubmitHeader, (uint32_t stream_id, const http2::HeaderPairs& headers), (override));

  int SubmitData(uint32_t stream_id, NoncontiguousBuffer&& data) override {
    return SubmitData_rvr(stream_id, std::move(data));
  }
  MOCK_METHOD(int, SubmitData_rvr, (uint32_t stream_id, NoncontiguousBuffer&& data));
  MOCK_METHOD(int, SubmitTrailer, (uint32_t stream_id, const http2::TrailerPairs& trailer), (override));
  MOCK_METHOD(int, SubmitReset, (uint32_t stream_id, uint32_t error_code), (override));

  MOCK_METHOD(bool, DecreaseRemoteWindowSize, (int32_t occupy_window_size), (override));

  void OnRequest(http2::RequestPtr&& request) {
    if (options_.on_eof_cb) {
      options_.on_eof_cb(request);
    }
  }

  void OnResponse(http2::ResponsePtr&& response) {
    if (options_.on_response_cb) {
      options_.on_response_cb(std::move(response));
    }
  }
};

using MockHttp2SessionPtr = std::shared_ptr<MockHttp2Session>;

inline MockHttp2SessionPtr CreateMockHttp2Session(http2::SessionOptions&& options) {
  return std::make_shared<MockHttp2Session>(std::move(options));
}

}  // namespace trpc::testing
