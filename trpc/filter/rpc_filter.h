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

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/message.h"
#include "rapidjson/document.h"

#include "trpc/common/status.h"
#include "trpc/filter/filter.h"
#include "trpc/filter/filter_point.h"
#include "trpc/util/flatbuffers/message_fbs.h"

namespace trpc {

/// @brief Encapsulate filters before and after server-side RPC calls to facilitate user inheritance and use.
///        It provide virtual interfaces for handling various specific request/response types, and users can override
///        the corresponding interfaces as needed.
class RpcServerFilter : public MessageServerFilter {
 public:
  using PbMessage = google::protobuf::Message;
  using FbsMessage = flatbuffers::trpc::MessageFbs;

  RpcServerFilter() = default;

  virtual ~RpcServerFilter() = default;

  std::vector<FilterPoint> GetFilterPoint() final {
    return std::vector<FilterPoint>{FilterPoint::SERVER_PRE_RPC_INVOKE,
                                    FilterPoint::SERVER_POST_RPC_INVOKE};
  }

  void operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) final;

 protected:
  /// @brief Pre-processing interface for void request/response type before RPC calls. It provides a default
  ///        implementation that includes the conversion of void* to the specific type and the invocation of the
  ///        corresponding BeforeRpcInvoke function.
  /// @note If this function returns a failure status, the filter chain will be interrupted.
  virtual Status BeforeRpcInvoke(ServerContextPtr context, void* req_raw, void* rsp_raw);

  /// @brief Post-processing interface for void request/response type after RPC calls. It provides a default
  ///        implementation that includes the conversion of void* to the specific type and the invocation of the
  ///        corresponding AfterRpcInvoke function.
  virtual Status AfterRpcInvoke(ServerContextPtr context, void* req_raw, void* rsp_raw);

  /// @brief Pre-processing interface for pb request/response type before RPC calls. The default implementation returns
  ///        a success status.
  /// @note If this function returns a failure status, the filter chain will be interrupted.
  virtual Status BeforeRpcInvoke(ServerContextPtr context, PbMessage* req, PbMessage* rsp) {
    return kSuccStatus;
  }

  /// @brief Post-processing interface for pb request/response type after RPC calls.
  virtual Status AfterRpcInvoke(ServerContextPtr context, PbMessage* req, PbMessage* rsp) {
    return kSuccStatus;
  }

  /// @brief Pre-processing interface for flatbuffer request/response type before RPC calls. The default implementation
  ///        returns a success status.
  /// @note If this function returns a failure status, the filter chain will be interrupted.
  virtual Status BeforeRpcInvoke(ServerContextPtr context, FbsMessage* req, FbsMessage* rsp) {
    return kSuccStatus;
  }

  /// @brief Post-processing interface for flatbuffer request/response type after RPC calls.
  virtual Status AfterRpcInvoke(ServerContextPtr context, FbsMessage* req, FbsMessage* rsp) {
    return kSuccStatus;
  }
};

/// @brief Encapsulate filters before and after client-side RPC calls to facilitate user inheritance and use.
///        It provide virtual interfaces for handling various specific request/response types, and users can override
///        the corresponding interfaces as needed.
///        Usage instructions are similar to RpcServerFilter.
class RpcClientFilter : public MessageClientFilter {
 public:
  using PbMessage = google::protobuf::Message;
  using FbsMessage = flatbuffers::trpc::MessageFbs;

  RpcClientFilter() = default;

  virtual ~RpcClientFilter() = default;

  std::vector<FilterPoint> GetFilterPoint() final {
    return std::vector<FilterPoint>{FilterPoint::CLIENT_PRE_RPC_INVOKE,
                                    FilterPoint::CLIENT_POST_RPC_INVOKE};
  }

  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) final;

 protected:
  virtual Status BeforeRpcInvoke(ClientContextPtr context, void* req_raw, void* rsp_raw);

  virtual Status AfterRpcInvoke(ClientContextPtr context, void* req_raw, void* rsp_raw);

  virtual Status BeforeRpcInvoke(ClientContextPtr context, PbMessage* req, PbMessage* rsp) {
    return kSuccStatus;
  }

  virtual Status AfterRpcInvoke(ClientContextPtr context, PbMessage* req, PbMessage* rsp) {
    return kSuccStatus;
  }

  virtual Status BeforeRpcInvoke(ClientContextPtr context, FbsMessage* req, FbsMessage* rsp) {
    return kSuccStatus;
  }

  virtual Status AfterRpcInvoke(ClientContextPtr context, FbsMessage* req, FbsMessage* rsp) {
    return kSuccStatus;
  }

  virtual Status BeforeRpcInvoke(ClientContextPtr context, rapidjson::Document* req,
                                 rapidjson::Document* rsp) {
    return kSuccStatus;
  }

  virtual Status AfterRpcInvoke(ClientContextPtr context, rapidjson::Document* req,
                                rapidjson::Document* rsp) {
    return kSuccStatus;
  }

  virtual Status BeforeRpcInvoke(ClientContextPtr context, std::string* req, std::string* rsp) {
    return kSuccStatus;
  }

  virtual Status AfterRpcInvoke(ClientContextPtr context, std::string* req, std::string* rsp) {
    return kSuccStatus;
  }
};

}  // namespace trpc
