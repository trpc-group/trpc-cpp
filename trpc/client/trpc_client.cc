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

#include "trpc/client/trpc_client.h"

namespace trpc {

void TrpcClient::Stop() { service_proxy_manager_.Stop(); }

void TrpcClient::Destroy() { service_proxy_manager_.Destroy(); }

std::shared_ptr<TrpcClient> GetTrpcClient() {
  static std::shared_ptr<TrpcClient> client = std::make_shared<TrpcClient>();
  return client;
}

}  // namespace trpc
