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

#include "examples/features/trpc_stream/server/stream_service_shopping.h"

#include <vector>

#include "gflags/gflags.h"

namespace test::shopping {

// Client streaming RPC.
::trpc::Status StreamShoppingServiceImpl::ClientStreamShopping(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::shopping::ShoppingRequest>& reader,
    ::trpc::test::shopping::ShoppingReply* reply) {
  ::trpc::Status status{};
  uint32_t request_counter{0};
  uint32_t request_bytes{0};
  for (;;) {
    ::trpc::test::shopping::ShoppingRequest request{};
    status = reader.Read(&request, 3000);
    if (status.OK()) {
      ++request_counter;
      request_bytes += request.msg().size();
      TRPC_FMT_INFO("server got request: {}", request.msg());
      continue;
    }
    if (status.StreamEof()) {
      std::stringstream reply_msg;
      reply_msg << "server got EOF, reply to client, server got request"
                << ", count:" << request_counter << ", received bytes:" << request_bytes;
      reply->set_msg(reply_msg.str());
      TRPC_FMT_INFO("reply to the client: {}", reply_msg.str());
      status = ::trpc::Status{0, 0, "OK"};
      break;
    }
    TRPC_FMT_ERROR("stream got error: {}", status.ToString());
    break;
  }
  return status;
}

// Server streaming RPC.
::trpc::Status StreamShoppingServiceImpl::ServerStreamShopping(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::test::shopping::ShoppingRequest& request,  // NO LINT
    ::trpc::stream::StreamWriter<::trpc::test::shopping::ShoppingReply>* writer) {
  ::trpc::Status status{};
  // A simple case, try to reply 10 response messages to the client.
  int request_count = 10;
  for (int i = 0; i < request_count; ++i) {
    std::stringstream reply_msg;
    reply_msg << " reply: " << request.msg() << "#" << (i + 1);
    ::trpc::test::shopping::ShoppingReply reply{};
    reply.set_msg(reply_msg.str());
    status = writer->Write(reply);
    if (status.OK()) {
      continue;
    }
    TRPC_FMT_ERROR("stream got error: {}", status.ToString());
    break;
  }
  return status;
}

// Bi-direction streaming RPC.
::trpc::Status StreamShoppingServiceImpl::BidiStreamShopping(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::shopping::ShoppingRequest>& reader,
    ::trpc::stream::StreamWriter<::trpc::test::shopping::ShoppingReply>* writer) {
  std::vector<std::string> msg_list{};
  ::trpc::Status status{};
  uint32_t request_counter{0};
  uint32_t request_bytes{0};
  for (;;) {
    ::trpc::test::shopping::ShoppingRequest request{};
    status = reader.Read(&request, 3000);
    if (status.OK()) {
      ++request_counter;
      request_bytes += request.msg().size();
      std::stringstream reply_msg;
      reply_msg << " reply:" << request_counter << ", received bytes:" << request_bytes;
      ::trpc::test::shopping::ShoppingReply reply;
      reply.set_msg(reply_msg.str());
      writer->Write(reply);
      continue;
    }
    if (status.StreamEof()) {
      std::stringstream reply_msg;
      reply_msg << "server got EOF, reply to client, server got request"
                << ", count:" << request_counter << ", received bytes:" << request_bytes;
      ::trpc::test::shopping::ShoppingReply reply;
      reply.set_msg(reply_msg.str());
      status = writer->Write(reply);
    }
    // ERROR.
    break;
  }
  if (!status.OK()) {
    TRPC_FMT_ERROR("stream go error: {}", status.ToString());
  }
  return status;
}

RawDataStreamService::RawDataStreamService() {
  AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
      "/trpc.test.shopping.RawDataStreamService/RawDataReadWrite", ::trpc::MethodType::BIDI_STREAMING,
      new ::trpc::StreamRpcMethodHandler<::trpc::NoncontiguousBuffer, ::trpc::NoncontiguousBuffer>(
          std::bind(&RawDataStreamService::RawDataReadWrite, this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3))));
}

::trpc::Status RawDataStreamService::RawDataReadWrite(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::NoncontiguousBuffer>& reader,  // NO LINT
    ::trpc::stream::StreamWriter<::trpc::NoncontiguousBuffer>* writer) {
  std::vector<std::string> msg_list{};
  ::trpc::Status status{};
  uint32_t request_counter{0};
  uint32_t request_bytes{0};
  for (;;) {
    ::trpc::NoncontiguousBuffer request{};
    status = reader.Read(&request, 3000);
    if (status.OK()) {
      ++request_counter;
      request_bytes += request.ByteSize();
      TRPC_FMT_INFO("Recv msg: {}", ::trpc::FlattenSlow(request));
      std::stringstream reply_msg;
      reply_msg << " reply:" << request_counter << ", received bytes:" << request_bytes;
      ::trpc::NoncontiguousBufferBuilder builder;
      builder.Append(reply_msg.str());
      writer->Write(builder.DestructiveGet());
      continue;
    }
    if (status.StreamEof()) {
      TRPC_FMT_INFO("Recv eof");
      std::stringstream reply_msg;
      reply_msg << "server got EOF, reply to client, server got request"
                << ", count:" << request_counter << ", received bytes:" << request_bytes;
      ::trpc::NoncontiguousBufferBuilder builder;
      builder.Append(reply_msg.str());
      status = writer->Write(builder.DestructiveGet());
    }
    // ERROR.
    break;
  }
  if (!status.OK()) {
    TRPC_FMT_ERROR("stream go error: {}", status.ToString());
  }
  return status;
}

}  // namespace test::shopping
