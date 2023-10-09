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
#include <string>
#include <vector>

#include "trpc/codec/grpc/http2/http2.h"
#include "trpc/codec/grpc/http2/session.h"

namespace trpc::http2 {

// @brief HTTP2 Client session.
class ClientSession : public Session {
 public:
  explicit ClientSession(Session::Options&& options);
  ~ClientSession() override;

  // @brief Initializes the session.
  bool Init() override;

  // @brief Submits a HTTP2 request.
  int SubmitRequest(const RequestPtr& request) override;

  // @brief Handles HTTP2 response.
  void OnResponse(StreamPtr& stream);

 private:
  // @brief Initializes the session, set callback methods, and set HTTP2 Settings.
  bool InitSession();

  // @brief Submits HTTP2 Settings information to negotiate stream control and other parameters.
  int SubmitSettings();
};

}  // namespace trpc::http2
