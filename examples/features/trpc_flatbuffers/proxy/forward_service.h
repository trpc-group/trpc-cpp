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

#include "examples/features/trpc_flatbuffers/proxy/forward.trpc.fb.h"
#include "examples/features/trpc_flatbuffers/proxy/greeter.trpc.fb.h"

namespace examples::flatbuffers {

class ForwardServiceImpl final : public ::trpc::test::forward::FbForward {
 public:
  ForwardServiceImpl();

  ::trpc::Status Forward(const ::trpc::ServerContextPtr& context,
                         const ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>* request,
                         ::flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* reply) override;

 private:
  std::shared_ptr<::trpc::test::helloworld::FbGreeterServiceProxy> proxy_;
};

}  // namespace examples::flatbuffers
