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

#include <any>
#include <functional>
#include <memory>
#include <string>

#ifdef TRPC_PROTO_USE_ARENA
#include "google/protobuf/arena.h"
#endif

#include "trpc/codec/protocol.h"
#include "trpc/common/status.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class ServerContext;
using ServerContextPtr = RefPtr<ServerContext>;

#ifdef TRPC_PROTO_USE_ARENA

/// @brief Set the ArenaOptions
///        the user can set this function to realize the function of customizing Arena's Options
///        if not set, use default option
void SetGlobalArenaOptions(const google::protobuf::ArenaOptions& options);

/// @brief Create an arena object, framework use
/// @private
google::protobuf::Arena GeneratePbArena();

/// @brief Create an arena object, framework use
/// @private
google::protobuf::Arena* GeneratePbArenaPtr();

#endif

/// @brief base class for rpc processing methods
class RpcMethodHandlerInterface {
 public:
  virtual ~RpcMethodHandlerInterface() = default;

  /// @brief Unary processing method
  /// @param context request context
  /// @param [in] req_data request buffer
  /// @param [out] rsp_data response buffer
  virtual void Execute(const ServerContextPtr& context, NoncontiguousBuffer&& req_data,
                       NoncontiguousBuffer& rsp_data) noexcept = 0;

  /// @brief Stream processing method
  /// @param context ServerContextPtr
  virtual void Execute(const ServerContextPtr& context) noexcept {}

  /// @brief Destroy the rpc request data structure object
  /// @param context ServerContext*
  virtual void DestroyReqObj(ServerContext* context) {}

  /// @brief Destroy the rpc response data structure object
  /// @param context ServerContext*
  virtual void DestroyRspObj(ServerContext* context) {}
};

/// @brief base class for non-rpc processing methods
class NonRpcMethodHandlerInterface {
 public:
  virtual ~NonRpcMethodHandlerInterface() = default;

  /// @brief unary processing method
  /// @param context request context
  /// @param[in] req request protocol object
  /// @param[out] rsp response protocol object
  virtual void Execute(const ServerContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) noexcept = 0;
};

}  // namespace trpc
