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

#include "trpc/codec/redis/redis_protocol.h"

#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/redis/reply.h"
#include "trpc/client/redis/request.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

namespace testing {

class RedisProtocolTest : public ::testing::Test {
 protected:
  redis::Request req_ = redis::Request();
  redis::Reply rsp_ = redis::Reply();
};

TEST_F(RedisProtocolTest, RequestTest) {
  redis::Request req;
  req.params_.push_back("SET");
  req.params_.push_back("trpc");
  req.params_.push_back("support_redis");

  RedisRequestProtocol redis_req_protocol = RedisRequestProtocol();
  redis_req_protocol.redis_req = std::move(req);
  NoncontiguousBuffer buff;
  ASSERT_FALSE(redis_req_protocol.ZeroCopyDecode(buff));
  ASSERT_TRUE(redis_req_protocol.ZeroCopyEncode(buff));
  BufferPtr buff_ptr = MakeRefCounted<Buffer>(buff.ByteSize());
  trpc::FlattenToSlow(buff, buff_ptr->GetWritePtr(), buff_ptr->WritableSize());
  buff_ptr->AddWriteLen(buff.ByteSize());
  std::stringstream out;
  out.write(buff_ptr->GetReadPtr(), buff_ptr->ReadableSize());
  std::string req_size_str("*");
  req_size_str += std::to_string(redis_req_protocol.redis_req.params_.size());
  std::string out_size_str;
  out >> out_size_str;
  ASSERT_EQ(req_size_str, out_size_str);

  for (auto& r : req.params_) {
    std::string param("$");
    param += std::to_string(r.size());
    std::string param_out;
    out >> param_out;
    ASSERT_EQ(param_out, param);
    out >> param_out;
    ASSERT_EQ(param_out, r);
  }

  redis::Request req_one = req_;
  req_one.params_.push_back("SET");
  req_one.do_RESP_ = false;
  redis_req_protocol.redis_req = std::move(req_one);

  NoncontiguousBuffer buff_one;
  ASSERT_TRUE(redis_req_protocol.ZeroCopyEncode(buff_one));
  redis_req_protocol.SetCallerName("test_caller");
  redis_req_protocol.SetCalleeName("test_callee");
  ASSERT_EQ(redis_req_protocol.GetCalleeName(), "test_callee");
  ASSERT_EQ(redis_req_protocol.GetCallerName(), "test_caller");
  BufferPtr buff_ptr2 = MakeRefCounted<Buffer>(buff_one.ByteSize());
  trpc::FlattenToSlow(buff_one, buff_ptr2->GetWritePtr(), buff_ptr2->WritableSize());
  buff_ptr2->AddWriteLen(buff_one.ByteSize());
  std::stringstream out_one;
  out_one.write(buff_ptr2->GetReadPtr(), buff_ptr2->ReadableSize());

  std::string out_one_str;
  out_one >> out_one_str;
  ASSERT_STREQ(out_one_str.c_str(), "SET");
}

TEST_F(RedisProtocolTest, ResponseTest) {
  redis::Reply rsp = rsp_;

  RedisResponseProtocol redis_rsp_protocol = RedisResponseProtocol();
  redis_rsp_protocol.redis_rsp = rsp;
  NoncontiguousBuffer buff;

  ASSERT_FALSE(redis_rsp_protocol.ZeroCopyDecode(buff));
  ASSERT_FALSE(redis_rsp_protocol.ZeroCopyEncode(buff));
}

}  // namespace testing

}  // namespace trpc
