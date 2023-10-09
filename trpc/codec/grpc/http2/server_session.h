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

class ServerSession : public Session {
 public:
  explicit ServerSession(Session::Options&& options);
  ~ServerSession() override;

  // @brief Initializes the session.
  bool Init() override;

  // @brief Notifies reading the frame message, read the message in the in buffer, and parse the received frame message.
  // @param in is streaming message received from the network.
  // @return 0: success; -1: error.
  int SignalRead(NoncontiguousBuffer* in) override;

  // @brief Submits a HTTP2 response.
  int SubmitResponse(const ResponsePtr& response) override;

  // @brief Handles HTTP2 header.
  void OnHeader(RequestPtr& request);

  // @brief Handles content chunk.
  void OnData(RequestPtr& request, const uint8_t* data, std::size_t len);

  // @brief Handles EOF.
  void OnEof(RequestPtr& request);

  // @brief Handles RESET.
  void OnRst(RequestPtr& request, uint32_t error_code);

  // @brief Handles window-updating.
  void OnWindowUpdate();

  // @brief Decreases the available window size for connection-level flow control.
  // @param occupy_window_size is the size of the window to be occupied (only actually occupied after submission).
  // @return true if the reduction is successful; false if the window size is insufficient.
  bool DecreaseRemoteWindowSize(int32_t occupy_window_size) override;

 private:
  // @brief Initializes the session, set callback methods, and set HTTP2 Settings.
  bool InitSession();

  // @brief Submits HTTP2 Settings information to negotiate stream control and other parameters.
  int SubmitSettings();

  // @brief Handles preface message received from client (Client magic message).
  bool HandleClientMagic(NoncontiguousBuffer* in);

 private:
  // Mark processing of client MAGIC message as successful.
  bool good_client_magic_{false};
  // The number of bytes that can still be sent on the connection (Currently only supports connection-level flow
  // control).
  int32_t remote_window_size_{0};
};

}  // namespace trpc::http2
