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

#include "trpc/codec/trpc/trpc_protocol.h"

#include <arpa/inet.h>

#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::testing {

namespace {
auto TrpcFixedHeaderComparator = [](const TrpcFixedHeader& lhs, const TrpcFixedHeader& rhs) {
  if (lhs.magic_value != rhs.magic_value || lhs.data_frame_type != rhs.data_frame_type ||
      lhs.stream_frame_type != rhs.stream_frame_type || lhs.data_frame_size != rhs.data_frame_size ||
      lhs.stream_id != rhs.stream_id || lhs.pb_header_size != rhs.pb_header_size) {
    return false;
  }

  return true;
};
}  // namespace

TEST(TrpcFixedHeader, TrpcFixedHeaderEncodeSuccess) {
  TrpcFixedHeader trpc_fixed_header;
  char buff[TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE] = {0};
  ASSERT_EQ(true, trpc_fixed_header.Encode(buff));
}

TEST(TrpcFixedHeader, TrpcFixedHeaderEncodeBufferNullptrFailed) {
  TrpcFixedHeader trpc_fixed_header;
  ASSERT_EQ(false, trpc_fixed_header.Encode(nullptr));
}

TEST(TrpcFixedHeaderTest, TrpcFixedHeaderDecodeSuccess) {
  NoncontiguousBufferBuilder builder;

  uint16_t magic_value = TrpcMagic::TRPC_MAGIC_VALUE;
  magic_value = htons(magic_value);
  builder.Append(&magic_value, TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE);

  uint8_t data_frame_type = 0;
  builder.Append(&data_frame_type, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE);

  uint8_t stream_frame_type = 0;
  builder.Append(&stream_frame_type, TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE);

  uint32_t data_frame_size = 100;
  data_frame_size = htonl(data_frame_size);
  builder.Append(&data_frame_size, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE);

  uint16_t pb_header_size = 20;
  pb_header_size = htons(pb_header_size);
  builder.Append(&pb_header_size, TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE);

  uint32_t stream_id = 100;
  stream_id = htonl(stream_id);
  builder.Append(&stream_id, TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE);

  char reversed[2] = {0};
  builder.Append(&reversed, TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE);

  TrpcFixedHeader trpc_fixed_header;
  auto buff = builder.DestructiveGet();

  ASSERT_TRUE(trpc_fixed_header.Decode(buff));
  ASSERT_EQ(trpc_fixed_header.magic_value, ntohs(magic_value));
  ASSERT_EQ(trpc_fixed_header.data_frame_type, data_frame_type);
  ASSERT_EQ(trpc_fixed_header.stream_frame_type, stream_frame_type);
  ASSERT_EQ(trpc_fixed_header.data_frame_size, ntohl(data_frame_size));
  ASSERT_EQ(trpc_fixed_header.pb_header_size, ntohs(pb_header_size));
  ASSERT_EQ(trpc_fixed_header.stream_id, ntohl(stream_id));
}

TEST(TrpcFixedHeader, TrpcFixedHeaderDecodeHeaderSizeFailed) {
  TrpcFixedHeader trpc_fixed_header;
  // Less length.
  auto buff = CreateBufferSlow("0123456789");
  ASSERT_EQ(false, trpc_fixed_header.Decode(buff));
}

TEST(TrpcFixedHeader, TrpcFixedHeaderDecodeMagicFailed) {
  NoncontiguousBufferBuilder builder;
  uint16_t magic_value = 100;
  magic_value = htons(magic_value);
  builder.Append(&magic_value, 2);

  TrpcFixedHeader trpc_fixed_header;

  auto buff = builder.DestructiveGet();
  ASSERT_EQ(false, trpc_fixed_header.Decode(buff));
}

TEST(TrpcFixedHeader, TrpcFixedHeaderEncodeAndDecodeOk) {
  TrpcFixedHeader fixed_header;
  fixed_header.data_frame_type = 1;
  fixed_header.stream_frame_type = 1;
  fixed_header.data_frame_size = 116;
  fixed_header.stream_id = 100;
  fixed_header.pb_header_size = 100;

  NoncontiguousBufferBuilder builder;
  auto* header_buffer = builder.Reserve(fixed_header.ByteSizeLong());

  ASSERT_TRUE(fixed_header.Encode(header_buffer));

  auto encoded_buffer = builder.DestructiveGet();
  TrpcFixedHeader decoded_fixed_header;

  // not skip header.
  ASSERT_TRUE(decoded_fixed_header.Decode(encoded_buffer, false));
  ASSERT_TRUE(TrpcFixedHeaderComparator(fixed_header, decoded_fixed_header));
  ASSERT_EQ(fixed_header.ByteSizeLong(), encoded_buffer.ByteSize());

  // skip header.
  decoded_fixed_header = TrpcFixedHeader{};

  ASSERT_TRUE(decoded_fixed_header.Decode(encoded_buffer, true));
  ASSERT_TRUE(TrpcFixedHeaderComparator(fixed_header, decoded_fixed_header));
  ASSERT_EQ(0, encoded_buffer.ByteSize());
}

size_t FillTrpcRequestProtocolDataWithoutAttachment(TrpcRequestProtocol& req) {
  req.fixed_header.magic_value = TrpcMagic::TRPC_MAGIC_VALUE;
  req.fixed_header.data_frame_type = 0;
  req.fixed_header.data_frame_type = 0;
  req.fixed_header.stream_frame_type = 0;
  req.fixed_header.data_frame_size = 0;
  req.fixed_header.pb_header_size = 0;
  req.fixed_header.stream_id = 0;

  req.req_header.set_version(0);
  req.req_header.set_call_type(0);
  req.req_header.set_request_id(1);
  req.req_header.set_timeout(1000);
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

size_t FillTrpcRequestProtocolDataWithAttachment(TrpcRequestProtocol& req) {
  std::string attachment_str("test attachment");
  req.req_header.set_attachment_size(attachment_str.size());
  size_t encode_buff_size = attachment_str.size();
  req.req_attachment = CreateBufferSlow(attachment_str);

  encode_buff_size += FillTrpcRequestProtocolDataWithoutAttachment(req);

  return encode_buff_size;
}

TEST(TrpcRequestProtocol, TrpcRequestProtocolEncodeSuccessWithoutAttachment) {
  TrpcRequestProtocol req;

  size_t encode_size = FillTrpcRequestProtocolDataWithoutAttachment(req);

  NoncontiguousBuffer buff;

  ASSERT_EQ(true, req.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);
}

TEST(TrpcRequestProtocol, TrpcRequestProtocolEncodeSuccessWithAttachment) {
  TrpcRequestProtocol req;

  size_t encode_size = FillTrpcRequestProtocolDataWithAttachment(req);

  NoncontiguousBuffer buff;

  ASSERT_EQ(true, req.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);
}

TEST(TrpcRequestProtocol, TrpcRequestProtocolDecodeSuccessWithoutAttachment) {
  TrpcRequestProtocol req;

  size_t encode_size = FillTrpcRequestProtocolDataWithoutAttachment(req);

  NoncontiguousBuffer buff;
  ASSERT_EQ(true, req.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);

  TrpcRequestProtocol temp;
  ASSERT_EQ(true, temp.ZeroCopyDecode(buff));
  ASSERT_EQ(buff.ByteSize(), 0);
}

TEST(TrpcRequestProtocol, TrpcRequestProtocolDecodeSuccessWithAttachment) {
  TrpcRequestProtocol req;

  size_t encode_size = FillTrpcRequestProtocolDataWithAttachment(req);

  NoncontiguousBuffer buff;
  ASSERT_EQ(true, req.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);

  TrpcRequestProtocol temp;
  ASSERT_EQ(true, temp.ZeroCopyDecode(buff));
  ASSERT_EQ(buff.ByteSize(), 0);
}

TEST(TrpcRequestProtocol, TrpcRequestProtocolDecodeFailure) {
  TrpcRequestProtocol req;

  size_t encode_size = FillTrpcRequestProtocolDataWithoutAttachment(req);

  NoncontiguousBuffer buff;
  ASSERT_EQ(true, req.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);
  buff.Append(CreateBufferSlow("hello"));

  TrpcRequestProtocol temp;
  ASSERT_FALSE(temp.ZeroCopyDecode(buff));
}

TEST(TrpcRequestProtocol, TrpcRequestProtocolId) {
  TrpcRequestProtocol req;
  uint32_t id_32 = 1;
  uint32_t id_res_32 = 0;

  req.SetRequestId(id_32);
  ASSERT_TRUE(req.GetRequestId(id_res_32));
  ASSERT_EQ(1, id_res_32);
}

size_t FillTrpcResponseProtocolDataWithoutAttachment(TrpcResponseProtocol& rsp) {
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

size_t FillTrpcResponseProtocolDataWithAttachment(TrpcResponseProtocol& rsp) {
  std::string attachment_str("test attachment");
  rsp.rsp_header.set_attachment_size(attachment_str.size());
  size_t encode_buff_size = attachment_str.size();
  rsp.rsp_attachment = CreateBufferSlow(attachment_str);

  encode_buff_size += FillTrpcResponseProtocolDataWithoutAttachment(rsp);

  return encode_buff_size;
}

TEST(TrpcResponseProtocol, TrpcResponseProtocolEncodeSuccessWithoutAttachment) {
  TrpcResponseProtocol rsp;

  size_t encode_size = FillTrpcResponseProtocolDataWithoutAttachment(rsp);

  NoncontiguousBuffer buff;

  ASSERT_EQ(true, rsp.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);
}

TEST(TrpcResponseProtocol, TrpcResponseProtocolEncodeSuccessWithAttachment) {
  TrpcResponseProtocol rsp;

  size_t encode_size = FillTrpcResponseProtocolDataWithAttachment(rsp);

  NoncontiguousBuffer buff;

  ASSERT_EQ(true, rsp.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);
}

TEST(TrpcResponseProtocol, TrpcResponseProtocolDecodeSuccessWithoutAttachment) {
  TrpcResponseProtocol rsp;

  size_t encode_size = FillTrpcResponseProtocolDataWithoutAttachment(rsp);

  NoncontiguousBuffer buff;

  ASSERT_EQ(true, rsp.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);

  TrpcResponseProtocol temp;
  ASSERT_EQ(true, rsp.ZeroCopyDecode(buff));
  ASSERT_EQ(buff.ByteSize(), 0);
}

TEST(TrpcResponseProtocol, TrpcResponseProtocolDecodeSuccessWithAttachment) {
  TrpcResponseProtocol rsp;

  size_t encode_size = FillTrpcResponseProtocolDataWithAttachment(rsp);

  NoncontiguousBuffer buff;

  ASSERT_EQ(true, rsp.ZeroCopyEncode(buff));
  ASSERT_EQ(buff.ByteSize(), encode_size);

  TrpcResponseProtocol temp;
  ASSERT_EQ(true, rsp.ZeroCopyDecode(buff));
  ASSERT_EQ(buff.ByteSize(), 0);
}

TEST(TrpcResponseProtocol, TrpcResponseProtocolId) {
  TrpcResponseProtocol rsp;
  uint32_t id_32 = 1;
  uint32_t id_res_32 = 0;

  rsp.SetRequestId(id_32);
  ASSERT_TRUE(rsp.GetRequestId(id_res_32));
  ASSERT_EQ(id_res_32, 1);
}

namespace {
size_t FillTrpcStreamInitFrameProtocol(TrpcStreamInitFrameProtocol* frame) {
  if (nullptr == frame) {
    return -1;
  }

  frame->fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
  frame->fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT;
  frame->fixed_header.data_frame_size = 0;
  frame->fixed_header.pb_header_size = 0;
  frame->fixed_header.stream_id = 100;

  frame->stream_init_metadata.mutable_request_meta()->set_caller("test_client");
  frame->stream_init_metadata.mutable_request_meta()->set_callee("trpc.test.helloworld.Greeter");
  frame->stream_init_metadata.mutable_request_meta()->set_func("/trpc.test.helloworld.Greeter/SayHello");

  frame->stream_init_metadata.mutable_response_meta()->set_ret(0);
  frame->stream_init_metadata.mutable_response_meta()->set_error_msg("OK");

  frame->stream_init_metadata.set_init_window_size(0);
  frame->stream_init_metadata.set_content_type(0);
  frame->stream_init_metadata.set_content_encoding(0);

  frame->fixed_header.data_frame_size = frame->ByteSizeLong();

  return frame->ByteSizeLong();
}
}  // namespace

namespace {
auto TrpcStreamInitFrameProtocolComparator = [](const TrpcStreamInitFrameProtocol& lhs,
                                                const TrpcStreamInitFrameProtocol& rhs) {
  if (!TrpcFixedHeaderComparator(lhs.fixed_header, rhs.fixed_header)) {
    return false;
  }

  if (lhs.stream_init_metadata.init_window_size() != rhs.stream_init_metadata.init_window_size() ||
      lhs.stream_init_metadata.content_type() != rhs.stream_init_metadata.content_type() ||
      lhs.stream_init_metadata.content_encoding() != rhs.stream_init_metadata.content_encoding()) {
    return false;
  }

  if (lhs.stream_init_metadata.request_meta().caller() != rhs.stream_init_metadata.request_meta().caller() ||
      lhs.stream_init_metadata.request_meta().callee() != rhs.stream_init_metadata.request_meta().callee() ||
      lhs.stream_init_metadata.request_meta().func() != rhs.stream_init_metadata.request_meta().func()) {
    return false;
  }

  if (lhs.stream_init_metadata.response_meta().ret() != rhs.stream_init_metadata.response_meta().ret() ||
      lhs.stream_init_metadata.response_meta().ret() != rhs.stream_init_metadata.response_meta().ret() ||
      lhs.stream_init_metadata.response_meta().error_msg() != rhs.stream_init_metadata.response_meta().error_msg()) {
    return false;
  }

  return true;
};
}  // namespace

TEST(TrpcStreamInitFrameProtocolTest, ZeroCopyEncode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamInitFrameProtocol stream_init_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamInitFrameProtocol{}, CreateBufferSlow("")},
      {
          "invalid data frame type",
          false,
          []() {
            TrpcStreamInitFrameProtocol init_frame;
            // Bad data frame type, expect:TRPC_STREAM_FRAME.
            init_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
            return init_frame;
          }(),
          CreateBufferSlow(""),
      },
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_init_frame.ZeroCopyEncode(t.buffer));
  }
}

TEST(TrpcStreamInitFrameProtocolTest, ZeroCopyDecode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamInitFrameProtocol stream_init_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamInitFrameProtocol{},
       []() {
         TrpcStreamInitFrameProtocol init_frame;
         FillTrpcStreamInitFrameProtocol(&init_frame);
         auto buffer = CreateBufferSlow("");
         init_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
      {"decoded fixed header failed", false, TrpcStreamInitFrameProtocol{}, CreateBufferSlow("")},
      {"invalid data frame type", false, TrpcStreamInitFrameProtocol{},
       []() {
         TrpcStreamInitFrameProtocol init_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         init_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         init_frame.fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT;
         auto buffer = CreateBufferSlow("");
         init_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_init_frame.ZeroCopyDecode(t.buffer)) << t.testing_name;
  }
}

TEST(TrpcStreamInitFrameProtocolTest, Check) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamInitFrameProtocol stream_init_frame;
  };

  std::vector<testing_args_t> testings{
      {"ok", true,
       []() {
         TrpcStreamInitFrameProtocol init_frame;
         FillTrpcStreamInitFrameProtocol(&init_frame);
         return init_frame;
       }()},
      {"invalid data frame type", false,
       []() {
         TrpcStreamInitFrameProtocol init_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         init_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         return init_frame;
       }()},
      {"invalid stream frame type", false,
       []() {
         TrpcStreamInitFrameProtocol init_frame;
         init_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
         // Bad stream frame type, expect:TRPC_STREAM_FRAME_INIT.
         init_frame.fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_UNARY;
         return init_frame;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_init_frame.Check()) << t.testing_name;
  }
}

TEST(TrpcStreamInitFrameProtocolTest, EncodeAndDecode) {
  TrpcStreamInitFrameProtocol init_frame;
  FillTrpcStreamInitFrameProtocol(&init_frame);

  auto encoded_buffer = CreateBufferSlow("");

  ASSERT_TRUE(init_frame.ZeroCopyEncode(encoded_buffer));

  TrpcStreamInitFrameProtocol decoded_init_frame;

  ASSERT_TRUE(decoded_init_frame.ZeroCopyDecode(encoded_buffer));
  ASSERT_TRUE(TrpcStreamInitFrameProtocolComparator(init_frame, decoded_init_frame));
}

namespace {
size_t FillTrpcStreamDataFrameProtocol(TrpcStreamDataFrameProtocol* frame) {
  if (nullptr == frame) {
    return -1;
  }

  frame->fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
  frame->fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_DATA;
  frame->fixed_header.data_frame_size = 0;
  frame->fixed_header.pb_header_size = 0;
  frame->fixed_header.stream_id = 100;

  frame->fixed_header.data_frame_size = frame->ByteSizeLong();

  return frame->ByteSizeLong();
}
}  // namespace

namespace {
auto TrpcStreamDataFrameProtocolComparator = [](const TrpcStreamDataFrameProtocol& lhs,
                                                const TrpcStreamDataFrameProtocol& rhs) {
  if (!TrpcFixedHeaderComparator(lhs.fixed_header, rhs.fixed_header)) {
    return false;
  }

  if (lhs.body.ByteSize() != rhs.body.ByteSize()) {
    return false;
  }

  return true;
};
}  // namespace

TEST(TrpcStreamDataFrameProtocolTest, ZeroCopyEncode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamDataFrameProtocol stream_data_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamDataFrameProtocol{}, CreateBufferSlow("")},
      {"invalid data frame type", false,
       []() {
         TrpcStreamDataFrameProtocol data_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         data_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         return data_frame;
       }(),
       CreateBufferSlow("")},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_data_frame.ZeroCopyEncode(t.buffer));
  }
}

TEST(TrpcStreamDataFrameProtocolTest, ZeroCopyDecode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamDataFrameProtocol stream_data_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamDataFrameProtocol{},
       []() {
         TrpcStreamDataFrameProtocol data_frame;
         FillTrpcStreamDataFrameProtocol(&data_frame);
         auto buffer = CreateBufferSlow("");
         data_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
      {"decode fixed header failed", false, TrpcStreamDataFrameProtocol{}, CreateBufferSlow("")},
      {"invalid data frame type", false, TrpcStreamDataFrameProtocol{},
       []() {
         TrpcStreamDataFrameProtocol data_frame;
         FillTrpcStreamDataFrameProtocol(&data_frame);
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         data_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         auto buffer = CreateBufferSlow("");
         data_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_data_frame.ZeroCopyDecode(t.buffer));
  }
}

TEST(TrpcStreamDataFrameProtocolTest, Check) {
  typedef struct {
    std::string testing_name;
    bool expect{true};

    TrpcStreamDataFrameProtocol stream_data_frame;
  } testing_args_t;

  std::vector<testing_args_t> testings{
      {"ok", true,
       []() {
         TrpcStreamDataFrameProtocol data_frame;
         FillTrpcStreamDataFrameProtocol(&data_frame);
         return data_frame;
       }()},
      {"invalid data frame type", false,
       []() {
         TrpcStreamDataFrameProtocol data_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         data_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         return data_frame;
       }()},
      {"invalid stream frame type", false,
       []() {
         TrpcStreamDataFrameProtocol data_frame;
         data_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
         // Bad stream frame type, expect:TRPC_STREAM_FRAME_DATA.
         data_frame.fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_UNARY;
         return data_frame;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_data_frame.Check());
  }
}

TEST(TrpcStreamDataFrameProtocolTest, EncodeAndDecode) {
  TrpcStreamDataFrameProtocol data_frame;
  FillTrpcStreamDataFrameProtocol(&data_frame);

  auto encoded_buffer = CreateBufferSlow("");
  ASSERT_TRUE(data_frame.ZeroCopyEncode(encoded_buffer));

  TrpcStreamDataFrameProtocol decoded_data_frame;
  ASSERT_TRUE(decoded_data_frame.ZeroCopyDecode(encoded_buffer));

  ASSERT_TRUE(TrpcStreamDataFrameProtocolComparator(data_frame, decoded_data_frame));
}

namespace {
size_t FillTrpcStreamFeedbackFrameProtocol(TrpcStreamFeedbackFrameProtocol* frame) {
  if (nullptr == frame) {
    return -1;
  }

  frame->fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
  frame->fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_FEEDBACK;
  frame->fixed_header.data_frame_size = 0;
  frame->fixed_header.pb_header_size = 0;
  frame->fixed_header.stream_id = 100;

  frame->stream_feedback_metadata.set_window_size_increment(999);

  frame->fixed_header.data_frame_size = frame->ByteSizeLong();

  return frame->ByteSizeLong();
}
}  // namespace

namespace {
auto TrpcStreamFeedbackFrameProtocolComparator = [](const TrpcStreamFeedbackFrameProtocol& lhs,
                                                    const TrpcStreamFeedbackFrameProtocol& rhs) {
  if (!TrpcFixedHeaderComparator(lhs.fixed_header, rhs.fixed_header)) {
    return false;
  }

  if (lhs.stream_feedback_metadata.window_size_increment() != rhs.stream_feedback_metadata.window_size_increment()) {
    return false;
  }

  return true;
};
}  // namespace

TEST(TrpcStreamFeedbackFrameProtocolTest, ZeroCopyEncode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamFeedbackFrameProtocol stream_feedback_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamFeedbackFrameProtocol{}, CreateBufferSlow("")},
      {"invalid data frame type", false,
       []() {
         TrpcStreamFeedbackFrameProtocol feedback_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         feedback_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         return feedback_frame;
       }(),
       CreateBufferSlow("")},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_feedback_frame.ZeroCopyEncode(t.buffer));
  }
}

TEST(TrpcStreamFeedbackFrameProtocolTest, ZeroCopyDecode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamFeedbackFrameProtocol stream_feedback_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamFeedbackFrameProtocol{},
       []() {
         TrpcStreamFeedbackFrameProtocol feedback_frame;
         FillTrpcStreamFeedbackFrameProtocol(&feedback_frame);
         auto buffer = CreateBufferSlow("");
         feedback_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
      {"decode fixed header failed", false, TrpcStreamFeedbackFrameProtocol{}, CreateBufferSlow("")},
      {"invalid data frame type", false, TrpcStreamFeedbackFrameProtocol{},
       []() {
         TrpcStreamFeedbackFrameProtocol feedback_frame;
         FillTrpcStreamFeedbackFrameProtocol(&feedback_frame);
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         feedback_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         auto buffer = CreateBufferSlow("");
         feedback_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_feedback_frame.ZeroCopyDecode(t.buffer));
  }
}

TEST(TrpcStreamFeedbackFrameProtocolTest, Check) {
  typedef struct {
    std::string testing_name;
    bool expect{true};

    TrpcStreamFeedbackFrameProtocol stream_feedback_frame;
  } testing_args_t;

  std::vector<testing_args_t> testings{
      {"ok", true,
       []() {
         TrpcStreamFeedbackFrameProtocol feedback_frame;
         FillTrpcStreamFeedbackFrameProtocol(&feedback_frame);
         return feedback_frame;
       }()},
      {"invalid data frame type", false,
       []() {
         TrpcStreamFeedbackFrameProtocol feedback_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         feedback_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         return feedback_frame;
       }()},
      {"invalid stream frame type", false,
       []() {
         TrpcStreamFeedbackFrameProtocol feedback_frame;
         feedback_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
         // Bad stream frame type, expect:TRPC_STREAM_FRAME_DATA.
         feedback_frame.fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_UNARY;
         return feedback_frame;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_feedback_frame.Check()) << t.testing_name;
  }
}

TEST(TrpcStreamFeedbackFrameProtocolTest, EncodeAndDecode) {
  TrpcStreamFeedbackFrameProtocol feedback_frame;
  FillTrpcStreamFeedbackFrameProtocol(&feedback_frame);

  auto encoded_buffer = CreateBufferSlow("");
  ASSERT_TRUE(feedback_frame.ZeroCopyEncode(encoded_buffer));

  TrpcStreamFeedbackFrameProtocol decoded_feedback_frame;
  ASSERT_TRUE(decoded_feedback_frame.ZeroCopyDecode(encoded_buffer));

  ASSERT_TRUE(TrpcStreamFeedbackFrameProtocolComparator(feedback_frame, decoded_feedback_frame));
}

namespace {
size_t FillTrpcStreamCloseFrameProtocol(TrpcStreamCloseFrameProtocol* frame) {
  if (nullptr == frame) {
    return -1;
  }

  frame->fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
  frame->fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_CLOSE;
  frame->fixed_header.data_frame_size = 0;
  frame->fixed_header.pb_header_size = 0;
  frame->fixed_header.stream_id = 100;

  frame->stream_close_metadata.set_close_type(TrpcStreamCloseType::TRPC_STREAM_CLOSE);
  frame->stream_close_metadata.set_ret(0);
  frame->stream_close_metadata.set_msg("OK");
  frame->stream_close_metadata.set_func_ret(0);

  frame->fixed_header.data_frame_size = frame->ByteSizeLong();

  return frame->ByteSizeLong();
}
}  // namespace

namespace {
auto TrpcStreamCloseFrameProtocolComparator = [](const TrpcStreamCloseFrameProtocol& lhs,
                                                 const TrpcStreamCloseFrameProtocol& rhs) {
  if (!TrpcFixedHeaderComparator(lhs.fixed_header, rhs.fixed_header)) {
    return false;
  }

  if (lhs.stream_close_metadata.close_type() != rhs.stream_close_metadata.close_type() ||
      lhs.stream_close_metadata.ret() != rhs.stream_close_metadata.ret() ||
      lhs.stream_close_metadata.msg() != rhs.stream_close_metadata.msg() ||
      lhs.stream_close_metadata.func_ret() != rhs.stream_close_metadata.func_ret()) {
    return false;
  }

  return true;
};
}  // namespace

TEST(TrpcStreamCloseFrameProtocolTest, ZeroCopyEncode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamCloseFrameProtocol stream_close_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamCloseFrameProtocol{}, CreateBufferSlow("")},
      {"invalid data frame type", false,
       []() {
         TrpcStreamCloseFrameProtocol close_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         close_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         return close_frame;
       }(),
       CreateBufferSlow("")},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_close_frame.ZeroCopyEncode(t.buffer));
  }
}

TEST(TrpcStreamCloseFrameProtocolTest, ZeroCopyDecode) {
  struct testing_args_t {
    std::string testing_name;
    bool expect{true};

    TrpcStreamCloseFrameProtocol stream_close_frame;
    NoncontiguousBuffer buffer;
  };

  std::vector<testing_args_t> testings{
      {"ok", true, TrpcStreamCloseFrameProtocol{},
       []() {
         TrpcStreamCloseFrameProtocol close_frame;
         FillTrpcStreamCloseFrameProtocol(&close_frame);
         auto buffer = CreateBufferSlow("");
         close_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
      {"decode fixed header failed", false, TrpcStreamCloseFrameProtocol{}, CreateBufferSlow("")},
      {"invalid data frame type", false, TrpcStreamCloseFrameProtocol{},
       []() {
         TrpcStreamCloseFrameProtocol close_frame;
         FillTrpcStreamCloseFrameProtocol(&close_frame);
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         close_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         auto buffer = CreateBufferSlow("");
         close_frame.ZeroCopyEncode(buffer);
         return buffer;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_close_frame.ZeroCopyDecode(t.buffer));
  }
}

TEST(TrpcStreamCloseFrameProtocolTest, Check) {
  typedef struct {
    std::string testing_name;
    bool expect{true};

    TrpcStreamCloseFrameProtocol stream_close_frame;
  } testing_args_t;

  std::vector<testing_args_t> testings{
      {"ok", true,
       []() {
         TrpcStreamCloseFrameProtocol close_frame;
         FillTrpcStreamCloseFrameProtocol(&close_frame);
         return close_frame;
       }()},
      {"invalid data frame type", false,
       []() {
         TrpcStreamCloseFrameProtocol close_frame;
         // Bad data frame type, expect:TRPC_STREAM_FRAME.
         close_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_UNARY_FRAME;
         return close_frame;
       }()},
      {"invalid stream frame type", false,
       []() {
         TrpcStreamCloseFrameProtocol close_frame;
         close_frame.fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
         // Bad stream frame type, expect:TRPC_STREAM_FRAME_DATA.
         close_frame.fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_UNARY;
         return close_frame;
       }()},
  };

  for (auto& t : testings) {
    ASSERT_EQ(t.expect, t.stream_close_frame.Check());
  }
}

TEST(TrpcStreamCloseFrameProtocolTest, EncodeAndDecode) {
  TrpcStreamCloseFrameProtocol close_frame;
  FillTrpcStreamCloseFrameProtocol(&close_frame);

  auto encoded_buffer = CreateBufferSlow("");
  ASSERT_TRUE(close_frame.ZeroCopyEncode(encoded_buffer));

  TrpcStreamCloseFrameProtocol decoded_close_frame;
  ASSERT_TRUE(decoded_close_frame.ZeroCopyDecode(encoded_buffer));

  ASSERT_TRUE(TrpcStreamCloseFrameProtocolComparator(close_frame, decoded_close_frame));
}

}  // namespace trpc::testing
