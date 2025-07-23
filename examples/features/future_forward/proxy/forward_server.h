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

#include "trpc/common/trpc_app.h"

namespace examples::forward {

class ForwardServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override;

  void Destroy() override;
};

}  // namespace examples::forward
