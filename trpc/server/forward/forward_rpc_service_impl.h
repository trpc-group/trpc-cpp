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

#include "trpc/server/service_impl.h"

namespace trpc {

/// @brief The implementation of Rpc transparent transmission service
class ForwardRpcServiceImpl : public ServiceImpl {
 public:
  void Dispatch(const ServerContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp)  noexcept override;
};

}  // namespace trpc
