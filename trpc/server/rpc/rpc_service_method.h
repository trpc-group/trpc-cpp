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

#include "trpc/server/method.h"
#include "trpc/server/method_handler.h"

namespace trpc {

/// @brief Rpc service method
class RpcServiceMethod : public Method {
 public:
  RpcServiceMethod(const std::string& name, MethodType type, RpcMethodHandlerInterface* handler)
      : Method(name, type), handler_(handler) {}

  ~RpcServiceMethod() override = default;

  RpcMethodHandlerInterface* GetRpcMethodHandler() const { return handler_.get(); }

 private:
  std::unique_ptr<RpcMethodHandlerInterface> handler_;
};

}  // namespace trpc
