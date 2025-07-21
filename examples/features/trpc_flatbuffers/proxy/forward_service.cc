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

#include "examples/features/trpc_flatbuffers/proxy/forward_service.h"

#include <memory>
#include <string>

#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/log/trpc_log.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

namespace examples::flatbuffers {

ForwardServiceImpl::ForwardServiceImpl() {
  proxy_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::FbGreeterServiceProxy>(
      "trpc.test.helloworld.FbGreeter");
}

::trpc::Status ForwardServiceImpl::Forward(
    const ::trpc::ServerContextPtr& context,
    const ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>* request,
    ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply>* reply) {
  auto client_context = ::trpc::MakeClientContext(context, proxy_);
  auto status = proxy_->SayHello(client_context, *request, reply);

  if (status.OK()) {
    ::flatbuffers::trpc::MessageBuilder mb;

    const ::trpc::test::helloworld::FbReply* rsp_msg = reply->GetRoot();
    auto msg_offset = mb.CreateString(rsp_msg->message()->str());
    auto hello_offset = ::trpc::test::helloworld::CreateFbReply(mb, msg_offset);
    mb.Finish(hello_offset);

    ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply> forward_rsp;
    forward_rsp = std::move(mb.ReleaseMessage<::trpc::test::helloworld::FbReply>());
    TRPC_ASSERT(forward_rsp.Verify());

    *reply = std::move(forward_rsp);
  }

  return status;
}

}  // namespace examples::flatbuffers
