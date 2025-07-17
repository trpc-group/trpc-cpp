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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define protected public

#include "trpc/client/redis/reader.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

namespace testing {

class ReaderTest : public ::testing::Test {
 public:
  void SetUp() { r_.Init(); }

  trpc::redis::Reader r_;
};

TEST_F(ReaderTest, SeekNewLine) {
  ASSERT_EQ('\r', *(r_.SeekNewLine("123456789\r\n", 11)));
  ASSERT_EQ('\r', *(r_.SeekNewLine("abc\r\r\r\r\r\r\n", 10)));
  ASSERT_EQ(nullptr, r_.SeekNewLine("abc\r\r\r\r\r\r\n", 9));
  ASSERT_EQ('\r', *(r_.SeekNewLine("abc\r\n", 5)));
  ASSERT_EQ('\r', *(r_.SeekNewLine("\r\n", 2)));
  ASSERT_EQ(nullptr, r_.SeekNewLine("abc", 3));
  ASSERT_EQ(nullptr, r_.SeekNewLine("abc\r", 4));
}

TEST_F(ReaderTest, ReadInteger) {
  ASSERT_EQ(1024, r_.ConvertToInteger("1024\r\n", 4));
  ASSERT_EQ(-1024, r_.ConvertToInteger("-1024\r\n", 5));
  ASSERT_EQ(1024, r_.ConvertToInteger("+1024\r\n", 5));
  ASSERT_EQ(0, r_.ConvertToInteger("-0\r\n", 2));
  ASSERT_EQ(0, r_.ConvertToInteger("0\r\n", 1));

  // Should not happend
  ASSERT_EQ(-1, r_.ConvertToInteger("-abc\r\n", 4));
}

TEST_F(ReaderTest, ReadIntegerNormal) {
  std::string s = "12345\r\ntest";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  size_t pos = 0;
  size_t len = 0;
  NoncontiguousBuffer::const_iterator begin_itr = in.begin();
  ASSERT_EQ('1', *(r_.ReadInteger(in, begin_itr, pos, len)));
  ASSERT_EQ(5, len);
  ASSERT_EQ(7, pos);
  len = 0;
  ASSERT_TRUE(begin_itr != in.end());
  ASSERT_EQ(nullptr, r_.ReadInteger(in, begin_itr, pos, len));
  ASSERT_EQ(0, len);
  ASSERT_EQ(7, pos);
}

TEST_F(ReaderTest, ReadIntegerEnd) {
  r_.Init();
  std::string s = "123456789\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  size_t len = 0;
  NoncontiguousBuffer::const_iterator begin_itr = in.begin();
  size_t pos = 0;
  ASSERT_EQ('1', *(r_.ReadInteger(in, begin_itr, pos, len)));
  ASSERT_EQ(0, pos);
  ASSERT_EQ(len, 9);
  ASSERT_FALSE(begin_itr != in.end());
}

TEST_F(ReaderTest, ReadIntegerCrossBuff) {
  r_.Init();
  std::string s = "123456789\r";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\na", 2);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  NoncontiguousBuffer::const_iterator begin_itr = in.begin();
  size_t len = 0;
  size_t pos = 0;
  ASSERT_EQ('1', *(r_.ReadInteger(in, begin_itr, pos, len)));
  ASSERT_EQ(pos, 1);
  ASSERT_EQ(len, 9);
  len = 0;
  ASSERT_TRUE(begin_itr != in.begin());
  ASSERT_EQ(nullptr, r_.ReadInteger(in, begin_itr, pos, len));
  ASSERT_EQ(len, 0);
  ASSERT_EQ(pos, 1);
  ASSERT_TRUE(begin_itr != in.end());
}

TEST_F(ReaderTest, ReadIntegerCrossBuff2) {
  r_.Init();
  std::string s = "123456789";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\r\na", 3);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  NoncontiguousBuffer::const_iterator begin_itr = in.begin();
  size_t len = 0;
  size_t pos = 0;
  ASSERT_EQ('1', *(r_.ReadInteger(in, begin_itr, pos, len)));
  ASSERT_EQ(pos, 2);
  ASSERT_EQ(len, 9);
  ASSERT_TRUE(begin_itr != in.end());
}

TEST_F(ReaderTest, ReadIntegerCrossBuff3) {
  r_.Init();
  std::string s = "123456789";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("7\r\n", 3);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  NoncontiguousBuffer::const_iterator begin_itr = in.begin();
  size_t len = 0;
  size_t pos = 0;
  ASSERT_EQ('1', *(r_.ReadInteger(in, begin_itr, pos, len)));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 10);
  ASSERT_FALSE(begin_itr != in.end());
}

TEST_F(ReaderTest, ReadIntegerCrossBuff4) {
  r_.Init();
  std::string s = "12345";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\r", 1);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  NoncontiguousBuffer::const_iterator begin_itr = in.begin();
  size_t len = 0;
  size_t pos = 0;
  ASSERT_EQ(nullptr, r_.ReadInteger(in, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 0);
  ASSERT_TRUE(begin_itr != in.end());
}

TEST_F(ReaderTest, ReadIntegerCrossBuff5) {
  struct alignas(hardware_destructive_interference_size) {
    BufferBuilder builder;
    NoncontiguousBuffer buffer;
  } read_buffer_;
  std::string s = "1234567\r\n";
  uint64_t wait_len = s.size();
  uint64_t pos = 0;
  while (wait_len > 0) {
    uint64_t append_len = 1;
    memcpy(read_buffer_.builder.data(), s.c_str() + pos, append_len);
    read_buffer_.buffer.Append(read_buffer_.builder.Seal(append_len));
    pos += append_len;
    wait_len -= append_len;
  }

  NoncontiguousBuffer::const_iterator begin_itr = read_buffer_.buffer.begin();
  size_t len = 0;
  pos = 0;
  ASSERT_EQ('1', *r_.ReadInteger(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());
}

TEST_F(ReaderTest, BadProtocol) {
  r_.Init();
  std::string s = "&foo\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_TRUE(!r);
  ASSERT_TRUE(r_.IsProtocolError());
}

TEST_F(ReaderTest, InvalidDepth) {
  r_.Init();
  std::string s = "*1\r\n";
  NoncontiguousBufferBuilder builder;
  for (int i = 0; i < 9; ++i) {
    builder.Append(s.c_str(), s.size());
  }
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_TRUE(!r);
  ASSERT_TRUE(r_.IsProtocolError());
}

TEST_F(ReaderTest, EmptyBulk) {
  r_.Init();
  std::string s = "*0\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsArray());
}

TEST_F(ReaderTest, EmptyReply) {
  r_.Init();
  NoncontiguousBufferBuilder builder;
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_TRUE(!r);
}

TEST_F(ReaderTest, ErrorReply) {
  r_.Init();
  std::string s = "-ERR\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsError());
  ASSERT_EQ("ERR", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("error", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StatusReply) {
  r_.Init();
  std::string s = "+OK\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsStatus());
  ASSERT_EQ("OK", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("status", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StatusReply2) {
  r_.Init();
  std::string s = "+OK\r";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\n", 1);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsStatus());
  ASSERT_EQ("OK", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("status", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StatusReply3) {
  r_.Init();
  std::string s = "+OK";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\r\n", 2);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsStatus());
  ASSERT_EQ("OK", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("status", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, IntegerReply) {
  r_.Init();
  std::string s = ":1024\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsInteger());
  ASSERT_EQ(1024, std::any_cast<trpc::redis::Reply>(out.front()).GetInteger());
  ASSERT_EQ("integer", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StringReply) {
  r_.Init();
  std::string s = "$4\r\ntrpc\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsString());
  ASSERT_EQ("trpc", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("string", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StringReply2) {
  r_.Init();
  std::string s = "$4\r\nt";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("rpc\r\n", 5);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsString());
  ASSERT_EQ("trpc", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("string", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StringReply3) {
  r_.Init();
  std::string s = "$4\r\ntrpc\r";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\n", 1);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsString());
  ASSERT_EQ("trpc", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("string", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StringReply4) {
  r_.Init();
  std::string s = "$4\r\ntrpc";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\r\n", 2);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsString());
  ASSERT_EQ("trpc", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("string", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StringReply5) {
  r_.Init();
  std::string s = "$4\r\ntrpc";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\r", 1);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(r);
  ASSERT_TRUE(out.empty());
  ASSERT_FALSE(r_.protocol_err_);
}

TEST_F(ReaderTest, StringReply6) {
  r_.Init();
  std::string s = "$4\r\ntrpc";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\r\na", 3);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsString());
  ASSERT_EQ("trpc", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("string", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StringReply7) {
  r_.Init();
  std::string s = "$4\r\ntrpc\r";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("\n", 1);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsString());
  ASSERT_EQ("trpc", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("string", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StringReply8) {
  struct alignas(hardware_destructive_interference_size) {
    BufferBuilder builder;
    NoncontiguousBuffer buffer;
  } read_buffer_;
  r_.Init();
  std::string s = "$4\r\ntrpc\r\n";
  memcpy(read_buffer_.builder.data(), s.c_str(), 8);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(8));
  memcpy(read_buffer_.builder.data(), s.c_str() + 8, 1);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(1));
  memcpy(read_buffer_.builder.data(), s.c_str() + 9, 1);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(1));
  std::deque<std::any> out;
  auto r = r_.GetReply(read_buffer_.buffer, out);
  ASSERT_FALSE(r_.IsProtocolError());
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsString());
  ASSERT_EQ("trpc", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("string", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, NilReply) {
  r_.Init();
  std::string s = "*-1\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsNil());
  ASSERT_EQ("nil", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, EmptyStringReply) {
  r_.Init();
  std::string s = "$-1\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_TRUE(r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsNil());
  ASSERT_EQ("nil", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, ArrayReply) {
  r_.Init();
  std::string s = "*3\r\n+OK\r\n*-1\r\n$4\r\ntrpc\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(s.c_str(), s.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsArray());
  ASSERT_EQ("array", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).GetArray().at(0).IsStatus());
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).GetArray().at(1).IsNil());
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).GetArray().at(2).IsString());
}

TEST_F(ReaderTest, MultiAppend) {
  r_.Init();
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - 14;
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append("*3\r\n+OK\r\n*-1\r\n", 14);
  builder.Append("$4\r\ntrpc\r\n", 10);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsArray());
  ASSERT_EQ("array", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).GetArray().at(0).IsStatus());
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).GetArray().at(1).IsNil());
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).GetArray().at(2).IsString());
}

TEST_F(ReaderTest, BadReply) {
  r_.Init();
  NoncontiguousBufferBuilder builder;
  builder.Append("-ERR\r", 5);
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_TRUE(!r);
}

TEST_F(ReaderTest, StatusReplyWithSerialNoFail) {
  r_.Init();
  NoncontiguousBufferBuilder builder;
  builder.Append("@12345", 6);
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_TRUE(!r);
}

TEST_F(ReaderTest, StatusReplyWithSerialNoFail2) {
  r_.Init();
  NoncontiguousBufferBuilder builder;
  builder.Append("@", 1);
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_TRUE(!r);
}

TEST_F(ReaderTest, StatusReplyWithSerialNo) {
  r_.Init();
  NoncontiguousBufferBuilder builder;
  builder.Append("@12345\r\n+OK\r\n", 13);
  NoncontiguousBuffer in = builder.DestructiveGet();
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_EQ(std::any_cast<trpc::redis::Reply>(out.front()).serial_no_, 12345);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsStatus());
  ASSERT_EQ("OK", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("status", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StatusReplyWithSerialNo2) {
  r_.Init();
  std::string s = "@123";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - s.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(s.c_str(), s.size());
  builder.Append("45\r\n+OK\r\n", 9);
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  auto r = r_.GetReply(in, out);
  ASSERT_FALSE(!r);
  ASSERT_EQ(std::any_cast<trpc::redis::Reply>(out.front()).serial_no_, 12345);
  ASSERT_TRUE(std::any_cast<trpc::redis::Reply>(out.front()).IsStatus());
  ASSERT_EQ("OK", std::any_cast<trpc::redis::Reply>(out.front()).GetString());
  ASSERT_EQ("status", std::any_cast<trpc::redis::Reply>(out.front()).TypeToString());
}

TEST_F(ReaderTest, StatusReplyWithSerialNo3) {
  r_.Init();
  std::string str1 = "@12345\r\n*2\r\n$3\r\nabc\r\n$";
  std::string str2 = "4\r\nedfb\r\n@12346\r\n*2\r\n$3\r\nabc\r\n$4\r\nedfb\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - str1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(str1.c_str(), str1.size());
  builder.Append(str2.c_str(), str2.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  bool r = r_.GetReply(in, out, 2);
  ASSERT_TRUE(r);
  ASSERT_EQ(out.size(), 1);
}

TEST_F(ReaderTest, StatusReplyWithSerialNo4) {
  r_.Init();
  std::string str1 = "@12345\r\n*2\r\n$3\r\nabc\r\n$";
  std::string str2 = "4\r\nedfb\r\n@12346\r\n*2\r\n$3\r\nabc\r\n$4\r\nedfb\r\n@123";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - str1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(str1.c_str(), str1.size());
  builder.Append(str2.c_str(), str2.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  bool r = r_.GetReply(in, out, 3);
  ASSERT_FALSE(r);
  ASSERT_EQ(out.size(), 0);
  ASSERT_FALSE(r_.IsProtocolError());
}

TEST_F(ReaderTest, pipeline) {
  r_.Init();
  std::string str1 = "+OK\r\n+QUEUED\r\n+QUEUED\r\n+QUEUED\r";
  std::string str2 = "\n+QUEUED\r\n+QUEUED*5\r\n+OK\r\n+OK\r\n+OK\r\n+OK\r\n$3\r\n333\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - str1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(str1.c_str(), str1.size());
  builder.Append(str2.c_str(), str2.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  bool r = r_.GetReply(in, out, 7);
  ASSERT_TRUE(r);
  ASSERT_EQ(out.size(), 1);
  ASSERT_FALSE(r_.IsProtocolError());
}

TEST_F(ReaderTest, SerialNoCrossBuffer) {
  r_.Init();
  std::string str1 = "@12345\r\n+OK\r\n@";
  std::string str2 = "23456\r\n+OK\r\n";
  NoncontiguousBufferBuilder builder;
  size_t fill_len = GetBlockMaxAvailableSize() - str1.size();
  std::string fill_str(fill_len, '\n');
  builder.Append(fill_str.c_str(), fill_len);
  builder.Append(str1.c_str(), str1.size());
  builder.Append(str2.c_str(), str2.size());
  NoncontiguousBuffer in = builder.DestructiveGet();
  in.Skip(fill_len);
  std::deque<std::any> out;
  bool r = r_.GetReply(in, out, 2);
  ASSERT_TRUE(r);
  ASSERT_EQ(out.size(), 1);
  ASSERT_FALSE(r_.IsProtocolError());
}

TEST_F(ReaderTest, TryReadVariableLengthString) {
  // "abc\rdef\r\n" may span cross multi block :
  // 1) “abc\r"、"d"、"ef\r\n"
  // 2）"abc"、"\r"、"def\r\n"
  // 2）"abc"、"\rdef\r\n"
  // 4）"abc\rdef\r\n"
  // 5）"abc\rdef\r"、"\n"
  // 6）"abc\rdef"、"\r\n"
  // 7）"abc\rdef"、"\r"、"\n"
  struct alignas(hardware_destructive_interference_size) {
    BufferBuilder builder;
    NoncontiguousBuffer buffer;
  } read_buffer_;

  std::string s = "abc\rdef\r\n";

  // “abc\r"、"d"、"ef\r\n"
  memcpy(read_buffer_.builder.data(), s.c_str(), 4);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(4));
  memcpy(read_buffer_.builder.data(), s.c_str() + 4, 1);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(1));
  memcpy(read_buffer_.builder.data(), s.c_str() + 5, 4);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(4));
  NoncontiguousBuffer::const_iterator begin_itr = read_buffer_.buffer.begin();
  size_t len = 0;
  size_t pos = 0;
  ASSERT_EQ(true, r_.TryReadVariableLengthString(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());

  // "abc"、"\r"、"def\r\n"
  read_buffer_.buffer.Skip(9);
  memcpy(read_buffer_.builder.data(), s.c_str(), 3);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(3));
  memcpy(read_buffer_.builder.data(), s.c_str() + 3, 1);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(1));
  memcpy(read_buffer_.builder.data(), s.c_str() + 4, 5);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(5));
  begin_itr = read_buffer_.buffer.begin();
  len = 0;
  pos = 0;
  ASSERT_EQ(true, r_.TryReadVariableLengthString(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());

  // "abc"、"\rdef\r\n"
  read_buffer_.buffer.Skip(9);
  memcpy(read_buffer_.builder.data(), s.c_str(), 3);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(3));
  memcpy(read_buffer_.builder.data(), s.c_str() + 3, 6);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(6));
  begin_itr = read_buffer_.buffer.begin();
  len = 0;
  pos = 0;
  ASSERT_EQ(true, r_.TryReadVariableLengthString(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());

  // "abc\rdef\r\n"
  read_buffer_.buffer.Skip(9);
  memcpy(read_buffer_.builder.data(), s.c_str(), 9);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(9));
  begin_itr = read_buffer_.buffer.begin();
  len = 0;
  pos = 0;
  ASSERT_EQ(true, r_.TryReadVariableLengthString(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());

  // "abc\rdef"、"\r\n"
  read_buffer_.buffer.Skip(9);
  memcpy(read_buffer_.builder.data(), s.c_str(), 7);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(7));
  memcpy(read_buffer_.builder.data(), s.c_str() + 7, 2);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(2));
  begin_itr = read_buffer_.buffer.begin();
  len = 0;
  pos = 0;
  ASSERT_EQ(true, r_.TryReadVariableLengthString(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());

  // "abc\rdef\r"、"\n"
  read_buffer_.buffer.Skip(9);
  memcpy(read_buffer_.builder.data(), s.c_str(), 8);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(8));
  memcpy(read_buffer_.builder.data(), s.c_str() + 8, 1);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(1));
  begin_itr = read_buffer_.buffer.begin();
  len = 0;
  pos = 0;
  ASSERT_EQ(true, r_.TryReadVariableLengthString(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());

  // "abc\rdef"、"\r"、"\n"
  read_buffer_.buffer.Skip(9);
  memcpy(read_buffer_.builder.data(), s.c_str(), 7);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(7));
  memcpy(read_buffer_.builder.data(), s.c_str() + 7, 1);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(1));
  memcpy(read_buffer_.builder.data(), s.c_str() + 8, 1);
  read_buffer_.buffer.Append(read_buffer_.builder.Seal(1));
  begin_itr = read_buffer_.buffer.begin();
  len = 0;
  pos = 0;
  ASSERT_EQ(true, r_.TryReadVariableLengthString(read_buffer_.buffer, begin_itr, pos, len));
  ASSERT_EQ(pos, 0);
  ASSERT_EQ(len, 7);
  ASSERT_FALSE(begin_itr != read_buffer_.buffer.end());
}
}  // namespace testing

}  // namespace trpc
