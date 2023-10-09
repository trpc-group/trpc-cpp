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

#include "examples/features/trpc_flatbuffers/server/greeter.trpc.fb.h"
#include "examples/features/trpc_flatbuffers/server/greeter_generated.h"

namespace examples::flatbuffers {

class DemoServiceImpl final : public ::trpc::test::helloworld::FbGreeter {
 public:
  ::trpc::Status SayHello(
      const ::trpc::ServerContextPtr& context,
      const ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>* request_msg,
      ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply>* response_msg) override;
};

}  // namespace examples::flatbuffers
