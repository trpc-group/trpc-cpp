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

#include <deque>
#include <iostream>
#include <string>
#include <utility>

#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/util/buffer/buffer.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::testing {

struct DummyTrpcProtocol {
  RpcCallType call_type = kUnaryCall;

  uint32_t request_id{1};

  uint32_t timeout{1000};

  std::string caller{"trpc.test.helloworld.client"};

  std::string callee{"trpc.test.helloworld.Greeter"};

  std::string func{"/trpc.test.helloworld.Greeter/SayHello"};

  TrpcMessageType message_type{TrpcMessageType::TRPC_DEFAULT};

  TrpcContentEncodeType content_type{TrpcContentEncodeType::TRPC_PROTO_ENCODE};

  TrpcCompressType content_encoding{TrpcCompressType::TRPC_DEFAULT_COMPRESS};

  serialization::DataType data_type{serialization::kPbMessage};
};

bool PackTrpcRequest(const DummyTrpcProtocol& protocol, void* in, NoncontiguousBuffer& bin_data) {
  serialization::SerializationType serialization_type = protocol.content_type;
  serialization::SerializationFactory* serializationfactory = serialization::SerializationFactory::GetInstance();

  std::cout << "pack trpc request" << std::endl;
  NoncontiguousBuffer body;
  bool encode_ret = serializationfactory->Get(serialization_type)->Serialize(protocol.data_type, in, &body);
  if (!encode_ret) {
    return false;
  }

  std::cout << "pack trpc request ok" << std::endl;

  TrpcRequestProtocol req;

  req.req_header.set_version(0);
  req.req_header.set_call_type(protocol.call_type);
  req.req_header.set_request_id(protocol.request_id);
  req.req_header.set_timeout(protocol.timeout);
  req.req_header.set_caller(protocol.caller);
  req.req_header.set_callee(protocol.callee);
  req.req_header.set_func(protocol.func);
  req.req_header.set_message_type(protocol.message_type);
  req.req_header.set_content_type(protocol.content_type);
  req.req_header.set_content_encoding(protocol.content_encoding);

  auto req_trans_info = req.req_header.mutable_trans_info();
  (*req_trans_info)["testkey1"] = "testvalue1";
  (*req_trans_info)["testkey2"] = "testvalue2";

  req.req_header.set_content_encoding(TrpcCompressType::TRPC_DEFAULT_COMPRESS);

  req.SetNonContiguousProtocolBody(std::move(body));

  std::cout << "pack trpc request end" << std::endl;

  return req.ZeroCopyEncode(bin_data);
}

bool UnPackTrpcResponse(NoncontiguousBuffer& buff, const DummyTrpcProtocol& protocol, void* out) {
  TrpcResponseProtocol rsp;
  if (rsp.ZeroCopyDecode(buff)) {
    NoncontiguousBuffer data;

    serialization::SerializationType serialization_type = protocol.content_type;
    serialization::SerializationFactory* serializationfactory = serialization::SerializationFactory::GetInstance();

    serialization::DataType out_type = protocol.data_type;

    return serializationfactory->Get(serialization_type)->Deserialize(&rsp.rsp_body, out_type, out);
  }

  return false;
}

bool UnPackTrpcResponseBody(NoncontiguousBuffer& buff, const DummyTrpcProtocol& protocol, void* out) {
  serialization::SerializationType serialization_type = protocol.content_type;
  serialization::SerializationFactory* serializationfactory = serialization::SerializationFactory::GetInstance();
  serialization::DataType out_type = protocol.data_type;

  return serializationfactory->Get(serialization_type)->Deserialize(&buff, out_type, out);
}


size_t FillTrpcRequestProtocolData(TrpcRequestProtocol& req, uint32_t timeout = 1000) {
  req.fixed_header.magic_value = TrpcMagic::TRPC_MAGIC_VALUE;
  req.fixed_header.data_frame_type = 0;
  req.fixed_header.stream_frame_type = 0;
  req.fixed_header.data_frame_size = 0;
  req.fixed_header.pb_header_size = 0;
  req.fixed_header.stream_id = 0;

  req.req_header.set_version(0);
  req.req_header.set_call_type(0);
  req.req_header.set_request_id(1);
  if (timeout > 0) {
    req.req_header.set_timeout(1000);
  }
  req.req_header.set_caller("test_client");
  req.req_header.set_callee("trpc.test.helloworld.Greeter");
  req.req_header.set_func("/trpc.test.helloworld.Greeter/SayHello");

  uint32_t req_header_size = req.req_header.ByteSizeLong();

  req.fixed_header.pb_header_size = req_header_size;

  std::string body_str("hello world");

  size_t encode_buff_size = TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + req_header_size + body_str.size();

  req.fixed_header.data_frame_size = encode_buff_size;

  req.req_body = CreateBufferSlow(body_str);

  return encode_buff_size;
}

size_t FillTrpcResponseProtocolData(TrpcResponseProtocol& rsp) {
  rsp.fixed_header.magic_value = TrpcMagic::TRPC_MAGIC_VALUE;
  rsp.fixed_header.data_frame_type = 0;
  rsp.fixed_header.data_frame_type = 0;
  rsp.fixed_header.stream_frame_type = 0;
  rsp.fixed_header.data_frame_size = 0;
  rsp.fixed_header.pb_header_size = 0;
  rsp.fixed_header.stream_id = 0;

  rsp.rsp_header.set_version(0);
  rsp.rsp_header.set_call_type(0);
  rsp.rsp_header.set_request_id(1);
  rsp.rsp_header.set_ret(0);
  rsp.rsp_header.set_func_ret(0);

  uint32_t rsp_header_size = rsp.rsp_header.ByteSizeLong();

  rsp.fixed_header.pb_header_size = rsp_header_size;

  std::string body_str("hello world");

  size_t encode_buff_size = TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + rsp_header_size + body_str.size();

  rsp.fixed_header.data_frame_size = encode_buff_size;

  rsp.rsp_body = CreateBufferSlow(body_str);

  return encode_buff_size;
}

}  // namespace trpc::testing
