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

#include "trpc/client/rpc_service_proxy.h"
#include "trpc/server/forward/forward_rpc_service_impl.h"
#include "trpc/util/buffer/buffer.h"

namespace examples::transparent {

class TransparentServiceImpl : public ::trpc::ForwardRpcServiceImpl {
 public:
  TransparentServiceImpl();

  // transparent transmission for reverse proxy
  ::trpc::Status Forward(::trpc::ServerContextPtr context, ::trpc::NoncontiguousBuffer&& request,
                         ::trpc::NoncontiguousBuffer& response);

 private:
  ::trpc::Status FiberForward(::trpc::ServerContextPtr context, ::trpc::NoncontiguousBuffer&& request,
                              ::trpc::NoncontiguousBuffer& response);
  ::trpc::Status FutureForward(::trpc::ServerContextPtr context, ::trpc::NoncontiguousBuffer&& request,
                               ::trpc::NoncontiguousBuffer& response);

 private:
  std::shared_ptr<trpc::RpcServiceProxy> proxy_ptr_;
};

}  // namespace examples::transparent
