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

#include "examples/features/trpc_flatbuffers/server/demo_service.h"

#include <memory>
#include <string>

#include "trpc/log/trpc_log.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

namespace examples::flatbuffers {

::trpc::Status DemoServiceImpl::SayHello(
    const ::trpc::ServerContextPtr& context,
    const ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>* request_msg,
    ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply>* response_msg) {
  ::flatbuffers::trpc::MessageBuilder mb;

  // We call GetRoot to "parse" the message. Verification is already
  // performed by default. See the notes below for more details.
  const ::trpc::test::helloworld::FbRequest* request = request_msg->GetRoot();

  // Fields are retrieved as usual with FlatBuffers
  const std::string& name = request->message()->str();

  TRPC_FMT_INFO("request msg:{}", name);

  // `flatbuffers::grpc::MessageBuilder` is a `FlatBufferBuilder` with a
  // special allocator for efficient gRPC buffer transfer, but otherwise
  // usage is the same as usual.
  auto msg_offset = mb.CreateString(name);
  auto hello_offset = ::trpc::test::helloworld::CreateFbReply(mb, msg_offset);
  mb.Finish(hello_offset);

  // The `ReleaseMessage<T>()` function detaches the message from the
  // builder, so we can transfer the resopnse to gRPC while simultaneously
  // detaching that memory buffer from the builer.
  *response_msg = mb.ReleaseMessage<::trpc::test::helloworld::FbReply>();
  TRPC_ASSERT(response_msg->Verify());

  // Return an OK status.
  return ::trpc::kSuccStatus;
}

}  // namespace examples::flatbuffers
