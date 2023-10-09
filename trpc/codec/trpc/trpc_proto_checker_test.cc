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

#include "trpc/codec/trpc/trpc_proto_checker.h"

#include <any>
#include <deque>
#include <string>

#include "gtest/gtest.h"

#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/runtime/iomodel/reactor/testing/mock_connection_testing.h"
#include "trpc/util/buffer/zero_copy_stream.h"

namespace trpc::testing {

class TrpcProtoCheckerTest : public ::testing::Test {
 protected:
  void SetUp() override { out.clear(); }

  static std::deque<std::any> out;
  static ConnectionPtr conn;
};

std::deque<std::any> TrpcProtoCheckerTest::out;
ConnectionPtr TrpcProtoCheckerTest::conn;

TEST_F(TrpcProtoCheckerTest, TrpcProtoCheckerFullPacket) {
  TrpcRequestProtocol req;

  size_t encode_size = FillTrpcRequestProtocolData(req);
  NoncontiguousBuffer buff;
  ASSERT_EQ(true, req.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);

  out.clear();
  auto result = trpc::CheckTrpcProtocolMessage(conn, buff, out);

  ASSERT_EQ(result, PacketChecker::PACKET_FULL);
  ASSERT_EQ(out.size(), 1);
  ASSERT_EQ(buff.ByteSize(), 0);
}

TEST_F(TrpcProtoCheckerTest, TrpcProtoCheckerPacketLess1) {
  out.clear();
  auto buff = CreateBufferSlow("0123456789");
  auto total_size = buff.ByteSize();

  auto result = trpc::CheckTrpcProtocolMessage(conn, buff, out);

  ASSERT_EQ(result, PacketChecker::PACKET_LESS);
  ASSERT_EQ(out.size(), 0);
  ASSERT_EQ(buff.ByteSize(), total_size);
}

TEST_F(TrpcProtoCheckerTest, TrpcProtoCheckerPACKETMAGICERR) {
  trpc::TrpcRequestProtocol req;

  req.req_header.set_version(0);
  req.req_header.set_call_type(0);
  req.req_header.set_request_id(1);
  req.req_header.set_timeout(1000);
  req.req_header.set_caller("test_client");
  req.req_header.set_callee("trpc.test.helloworld.Greeter");
  req.req_header.set_func("/trpc.test.helloworld.Greeter/SayHello");

  uint32_t req_header_size = req.req_header.ByteSizeLong();

  std::string body_str("hello world");

  uint32_t req_body_size = body_str.size();

  NoncontiguousBufferBuilder builder;
  uint16_t magic_value = 0;
  magic_value = htons(magic_value);

  builder.Append(&magic_value, TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE);

  builder.Append(&(req.fixed_header.data_frame_type), TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE);

  builder.Append(&(req.fixed_header.stream_frame_type), TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE);

  uint32_t total_size = TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + req_header_size + req_body_size;
  uint32_t data_frame_size = total_size;
  data_frame_size = htonl(data_frame_size);
  builder.Append(&data_frame_size, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE);

  uint16_t pb_header_size = req_header_size;
  pb_header_size = htons(req_header_size);

  builder.Append(&pb_header_size, TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE);

  uint32_t stream_id = req.fixed_header.stream_id;
  stream_id = htonl(stream_id);
  builder.Append(&stream_id, TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE);

  builder.Append(&(req.fixed_header.reversed), TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE);

  NoncontiguousBufferOutputStream nbos(&builder);
  ASSERT_TRUE(req.req_header.SerializePartialToZeroCopyStream(&nbos));
  nbos.Flush();

  builder.Append(body_str);

  out.clear();

  auto buff = builder.DestructiveGet();
  ASSERT_EQ(buff.ByteSize(), total_size);

  auto result = trpc::CheckTrpcProtocolMessage(conn, buff, out);

  ASSERT_EQ(result, PacketChecker::PACKET_ERR);
  ASSERT_EQ(out.size(), 0);
  ASSERT_EQ(buff.ByteSize(), total_size);
}

TEST_F(TrpcProtoCheckerTest, TrpcProtoCheckerPACKEDATAFRAMESIZETERR1) {
  trpc::TrpcRequestProtocol req;

  req.req_header.set_version(0);
  req.req_header.set_call_type(0);
  req.req_header.set_request_id(1);
  req.req_header.set_timeout(1000);
  req.req_header.set_caller("test_client");
  req.req_header.set_callee("trpc.test.helloworld.Greeter");
  req.req_header.set_func("/trpc.test.helloworld.Greeter/SayHello");

  uint32_t req_header_size = req.req_header.ByteSizeLong();

  std::string body_str("hello world");

  uint32_t req_body_size = body_str.size();

  NoncontiguousBufferBuilder builder;
  uint16_t magic_value = req.fixed_header.magic_value;
  magic_value = htons(magic_value);

  builder.Append(&magic_value, TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE);

  builder.Append(&(req.fixed_header.data_frame_type), TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE);

  builder.Append(&(req.fixed_header.stream_frame_type), TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE);

  uint32_t total_size = TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + req_header_size + req_body_size;
  uint32_t data_frame_size = 10;
  data_frame_size = htonl(data_frame_size);
  builder.Append(&data_frame_size, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE);

  uint16_t pb_header_size = req_header_size;
  pb_header_size = htons(req_header_size);

  builder.Append(&pb_header_size, TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE);

  uint32_t stream_id = req.fixed_header.stream_id;
  stream_id = htonl(stream_id);
  builder.Append(&stream_id, TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE);

  builder.Append(&(req.fixed_header.reversed), TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE);

  NoncontiguousBufferOutputStream nbos(&builder);
  ASSERT_TRUE(req.req_header.SerializePartialToZeroCopyStream(&nbos));
  nbos.Flush();

  builder.Append(body_str);

  out.clear();

  auto buff = builder.DestructiveGet();
  ASSERT_EQ(buff.ByteSize(), total_size);

  auto result = trpc::CheckTrpcProtocolMessage(conn, buff, out);

  ASSERT_EQ(result, PacketChecker::PACKET_ERR);
  ASSERT_EQ(out.size(), 0);
  ASSERT_EQ(buff.ByteSize(), total_size);
}

TEST_F(TrpcProtoCheckerTest, TrpcProtoCheckerPACKEDATAFRAMESIZETERR2) {
  trpc::TrpcRequestProtocol req;

  req.req_header.set_version(0);
  req.req_header.set_call_type(0);
  req.req_header.set_request_id(1);
  req.req_header.set_timeout(1000);
  req.req_header.set_caller("test_client");
  req.req_header.set_callee("trpc.test.helloworld.Greeter");
  req.req_header.set_func("/trpc.test.helloworld.Greeter/SayHello");

  uint32_t req_header_size = req.req_header.ByteSizeLong();

  std::string body_str("hello world");

  uint32_t req_body_size = body_str.size();

  NoncontiguousBufferBuilder builder;
  uint16_t magic_value = req.fixed_header.magic_value;
  magic_value = htons(magic_value);

  builder.Append(&magic_value, TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE);

  builder.Append(&(req.fixed_header.data_frame_type), TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE);

  builder.Append(&(req.fixed_header.stream_frame_type), TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE);

  uint32_t total_size = TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + req_header_size + req_body_size;
  uint32_t data_frame_size = 10000;
  data_frame_size = htonl(data_frame_size);
  builder.Append(&data_frame_size, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE);

  uint32_t stream_id = req.fixed_header.stream_id;
  stream_id = htonl(stream_id);
  builder.Append(&stream_id, TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE);

  uint16_t pb_header_size = req_header_size;
  pb_header_size = htons(req_header_size);
  builder.Append(&pb_header_size, TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE);

  builder.Append(&(req.fixed_header.reversed), TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE);

  NoncontiguousBufferOutputStream nbos(&builder);
  ASSERT_TRUE(req.req_header.SerializePartialToZeroCopyStream(&nbos));
  nbos.Flush();

  builder.Append(body_str);

  out.clear();

  auto buff = builder.DestructiveGet();
  ASSERT_EQ(buff.ByteSize(), total_size);

  conn = MakeRefCounted<trpc::testing::MockConnection>();
  conn->SetConnType(ConnectionType::kTcpLong);
  auto result = trpc::CheckTrpcProtocolMessage(conn, buff, out);

  ASSERT_EQ(result, PacketChecker::PACKET_LESS);
  ASSERT_EQ(out.size(), 0);
  ASSERT_EQ(buff.ByteSize(), total_size);
}

TEST_F(TrpcProtoCheckerTest, TrpcProtoCheckerPACKETLESS2) {
  trpc::TrpcRequestProtocol req;

  req.req_header.set_version(0);
  req.req_header.set_call_type(0);
  req.req_header.set_request_id(1);
  req.req_header.set_timeout(1000);
  req.req_header.set_caller("test_client");
  req.req_header.set_callee("trpc.test.helloworld.Greeter");
  req.req_header.set_func("/trpc.test.helloworld.Greeter/SayHello");

  uint32_t req_header_size = req.req_header.ByteSizeLong();

  std::string body_str("hello world");

  uint32_t req_body_size = body_str.size();

  NoncontiguousBufferBuilder builder;
  uint16_t magic_value = req.fixed_header.magic_value;
  magic_value = htons(magic_value);

  builder.Append(&magic_value, TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE);

  builder.Append(&(req.fixed_header.data_frame_type), TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE);

  builder.Append(&(req.fixed_header.stream_frame_type), TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE);

  uint32_t total_size = TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + req_header_size + req_body_size;
  uint32_t data_frame_size = total_size;
  data_frame_size = htonl(data_frame_size);
  builder.Append(&data_frame_size, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE);

  uint16_t pb_header_size = req_header_size;
  pb_header_size = htons(req_header_size);

  builder.Append(&pb_header_size, TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE);

  uint32_t stream_id = req.fixed_header.stream_id;
  stream_id = htonl(stream_id);
  builder.Append(&stream_id, TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE);

  builder.Append(&(req.fixed_header.reversed), TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE);

  NoncontiguousBufferOutputStream nbos(&builder);
  ASSERT_TRUE(req.req_header.SerializePartialToZeroCopyStream(&nbos));
  nbos.Flush();

  // builder.Append(body_str);

  out.clear();

  auto buff = builder.DestructiveGet();
  ASSERT_EQ(buff.ByteSize(), total_size - body_str.size());

  auto result = trpc::CheckTrpcProtocolMessage(conn, buff, out);

  ASSERT_EQ(result, PacketChecker::PACKET_LESS);
  ASSERT_EQ(out.size(), 0);
  ASSERT_EQ(buff.ByteSize(), total_size - body_str.size());
}

TEST(PickTrpcProtocolMessageMetadataTest, CheckOk) {
  TrpcFixedHeader fixed_header;
  fixed_header.data_frame_type = 1;
  fixed_header.stream_frame_type = 1;
  fixed_header.stream_id = 100;

  NoncontiguousBufferBuilder builder;
  auto header_buffer = builder.Reserve(fixed_header.ByteSizeLong());
  ASSERT_TRUE(fixed_header.Encode(header_buffer));

  std::any message = builder.DestructiveGet();
  std::any meta = trpc::TrpcProtocolMessageMetadata{};
  ASSERT_TRUE(trpc::PickTrpcProtocolMessageMetadata(message, meta));

  auto& metadata = std::any_cast<trpc::TrpcProtocolMessageMetadata&>(meta);
  ASSERT_EQ(fixed_header.data_frame_type, metadata.data_frame_type);
  ASSERT_EQ(fixed_header.stream_frame_type, metadata.stream_frame_type);
  ASSERT_EQ(fixed_header.stream_id, metadata.stream_id);
  ASSERT_TRUE(metadata.enable_stream);
}

TEST(PickTrpcProtocolMessageMetadataTest, CheckFailed) {
  std::any message = CreateBufferSlow("");
  std::any meta = TrpcProtocolMessageMetadata{};
  ASSERT_FALSE(PickTrpcProtocolMessageMetadata(message, meta));
}

}  // namespace trpc::testing
