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

#include "trpc/codec/redis/redis_proto_checker.h"

#include <any>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"

namespace trpc {

namespace testing {

class MockConnectionHandler : public ConnectionHandler {
 public:
  MockConnectionHandler(Connection* conn) { conn_ = conn; }

  Connection* GetConnection() const { return conn_; }

  uint32_t GetMergeRequestCount() override { return pipeline_count_; }

  void SetPipelineCount(uint32_t count) { pipeline_count_ = count; }

  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    return PacketChecker::PACKET_FULL;
  }

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) override { return true; }

 private:
  Connection* conn_;
  uint32_t pipeline_count_ = 1;
};

class RedisClientProtoCheckerTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {}

  static void TearDownTestCase() {}

  void SetUp() {
    in_.Clear();
    out_.clear();
  }

 public:
  NoncontiguousBuffer in_;
  std::deque<std::any> out_;
};

TEST_F(RedisClientProtoCheckerTest, SingleFullPacket) {
  std::string s = "*2\r\n$3\r\nabc\r\n$4\r\ntrpc\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, SingleFullPacket2) {
  std::string s = "*2\r\n$3\r\njio\r\n$3\r\niop\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, SingleNotFullPacket) {
  std::string s = "-ERR\r";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);

  EXPECT_EQ(PacketChecker::PACKET_LESS, result);
  EXPECT_EQ(0, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, MultiFullPacket) {
  std::string s = "*2\r\n$3\r\nabc\r\n$4\r\ntrpc\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  builder.Append(s.c_str(), s.size());
  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(3);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_TRUE(result);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, MultiNotFullPacket) {
  std::string s = "*2\r\n$3\r\nabc\r\n$4\r\ntrpc\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  builder.Append(s.c_str(), s.size());
  builder.Append(s.c_str(), 10);
  in_ = builder.DestructiveGet();
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(2);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);

  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
  EXPECT_EQ(10, in_.ByteSize());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket) {
  out_.clear();
  std::string s = "*2\r\n$3\r\nabc\r\n$4\r\ntrpc\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - 10;
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), 10);
  builder.Append(s.c_str() + 10, s.size() - 10);
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(1);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket2) {
  std::string s1 = "+OK\r\n+QUEUED\r\n+QUEUED\r\n+QUEUED\r\n";
  std::string s2 = "+QUEUED\r\n+QUEUED\r\n*5\r\n+OK\r\n+OK\r\n+OK\r\n+OK\r\n$3\r\n333\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  builder.Append(s2.c_str(), s2.size());
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(7);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket3) {
  std::string s1 = "*2\r\n$3";
  std::string s2 = "\r\njio\r\n$3\r\niop\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  builder.Append(s2.c_str(), s2.size());
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(1);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket4) {
  std::string s1 = "*2\r\n$3\r";
  std::string s2 = "\njio\r\n$3\r\niop\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  builder.Append(s2.c_str(), s2.size());
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(1);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket5) {
  std::string s1 = "*2\r\n$3\r\njio\r\n$3\r\niop\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - 12;
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), 12);
  builder.Append(s1.c_str() + 12, 10);
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(1);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket6) {
  std::string s1 = "*5\r\n+OK\r\n+OK\r\n+OK\r\n+OK\r\n$3\r\n333\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - 23;
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), 23);
  builder.Append(s1.c_str() + 23, 10);
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(1);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);

  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket7) {
  std::string s1 = "*2\r";
  std::string s2 = "\n$3\r\njio\r\n$3\r\niop\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  builder.Append(s2.c_str(), s2.size());
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(1);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket8) {
  std::string s1 = "+OK\r";
  std::string s2 = "\n+OK\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  builder.Append(s2.c_str(), 1);
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(1);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  out_.clear();
  NoncontiguousBufferBuilder builder2;
  builder2.Append(s2.c_str() + 1, 5);
  in_ = builder2.DestructiveGet();
  result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket9) {
  std::string s1 = "@123456\r\n+OK\r";
  std::string s2 =
      "\n+QUEUED\r\n+QUEUED\r\n+QUEUED\r\n+QUEUED\r\n+QUEUED\r\n*5\r\n+OK\r\n+OK\r\n+OK\r\n+OK\r\n$"
      "3\r\n333\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  builder.Append(s2.c_str(), s2.size());
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(7);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket10) {
  std::string s1 = "@123456\r\n+OK\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(7);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_LESS, result);
  in_.Clear();
  std::string s2 =
      "+QUEUED\r\n+QUEUED\r\n+QUEUED\r\n+QUEUED\r\n+QUEUED\r\n*5\r\n+OK\r\n+OK\r\n+OK\r\n+OK\r\n$"
      "3\r\n333\r\n";
  NoncontiguousBufferBuilder builder2;
  builder2.Append(fill_str.c_str(), fill_len);
  builder2.Append(s1.c_str(), s1.size());
  builder2.Append(s2.c_str(), s2.size());
  in_ = builder2.DestructiveGet();
  in_.Skip(fill_len);
  result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_FULL, result);
  EXPECT_EQ(1, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket11) {
  std::string s1 = "@123456\r\n$10\r\nabcdefghij\r\n$10\r\nabcdefghi";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - 11;
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), 11);
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(2);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_LESS, result);
  builder.Append(s1.c_str() + 11, 29);
  result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_LESS, result);
  EXPECT_EQ(0, out_.size());
}

TEST_F(RedisClientProtoCheckerTest, CrossFullPacket12) {
  in_.Clear();
  out_.clear();
  std::string s1 = "@214\r\n*12\r\n$19\r\nTWF2021011100697700\r\n";
  std::string s2 =
      "$24\r\n%08%E6%97%F1%FF%05%10%0A\r\n$19\r\nTWF2021011100220000\r\n$24\r\n%08%EF%A5%F1%FF%05%"
      "10%0A\r\n$16\r\n20210111A04AVV00\r\n$24\r\n%08%9A%D7%F1%FF%05%10%0A\r\n$"
      "19\r\nCEG2021011200653100\r\n$24\r\n%08%D6%C4%F6%FF%05%10%0A\r\n$16\r\n20210112A0EVGQ00\r\n$"
      "24\r\n%08%9F%D9%F6%FF%05%10%0A\r\n$16\r\n20210111A0FG0900\r\n$24\r\n%08%B6%F5%F6%FF%05%10%"
      "0A\r\n@215\r\n*12\r\n$19\r\nTWF2021011100697700\r\n$24";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s1.c_str(), s1.size());
  in_ = builder.DestructiveGet();
  in_.Skip(fill_len);
  ConnectionPtr conn_ptr = MakeRefCounted<Connection>();
  auto mock_conn_handler = std::make_unique<MockConnectionHandler>(conn_ptr.get());
  mock_conn_handler->SetPipelineCount(2);
  conn_ptr->SetConnectionHandler(std::move(mock_conn_handler));
  auto result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_LESS, result);
  EXPECT_EQ(37, in_.ByteSize());
  in_.Clear();
  NoncontiguousBufferBuilder builder2;
  builder2.Append(fill_str.c_str(), fill_len);
  builder2.Append(s1.c_str(), s1.size());
  builder2.Append(s2.c_str(), s2.size());
  in_ = builder2.DestructiveGet();
  in_.Skip(fill_len);
  result = trpc::RedisZeroCopyCheckResponse(conn_ptr, in_, out_);
  EXPECT_EQ(PacketChecker::PACKET_LESS, result);
  EXPECT_EQ(0, out_.size());
  EXPECT_EQ(384, in_.ByteSize());
}

}  // namespace testing

}  // namespace trpc
