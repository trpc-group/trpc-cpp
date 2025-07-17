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

#include "trpc/stream/stream.h"

#include "gtest/gtest.h"

#include "trpc/serialization/serialization_factory.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/stream/testing/mock_stream_provider.h"
#include "trpc/stream/testing/stream.pb.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

namespace {
using HelloRequest = trpc::test::helloworld::HelloRequest;
using HelloReply = trpc::test::helloworld::HelloReply;
}  // namespace

namespace {
serialization::Serialization* kPbSerialization = []() {
  serialization::Init();
  serialization::SerializationType pb = serialization::kPbType;

  return serialization::SerializationFactory::GetInstance()->Get(pb);
}();
serialization::Serialization* kNoopSerialization = []() {
  serialization::SerializationType noop = serialization::kNoopType;
  return serialization::SerializationFactory::GetInstance()->Get(noop);
}();
}  // namespace

namespace {
template <typename T>
bool Serialize(T& msg, NoncontiguousBuffer* buffer) {
  void* msg_ptr = static_cast<void*>(&msg);
  uint32_t content_type = 0;  // TrpcContentEncodeType::TRPC_PROTO_ENCODE

  return kPbSerialization->Serialize(content_type, msg_ptr, buffer);
}

template <typename T>
bool Deserialize(NoncontiguousBuffer* buffer, T* msg) {
  void* msg_ptr = static_cast<void*>(&msg);
  uint32_t content_type = 0;  // TrpcContentEncodeType::TRPC_PROTO_ENCODE

  return kPbSerialization->Deserialize(content_type, buffer, msg);
}
}  // namespace

namespace {
template <typename W, typename R>
struct MessageReaderWriter {
  W w_msg{};
  R r_msg{};
  Status status{};
  bool read_good{true};

  Status Read(NoncontiguousBuffer* msg, int timeout = -1) {
    if (read_good) {
      Serialize(r_msg, msg);
    } else {
      *msg = CreateBufferSlow("Hello World");
    }

    return status;
  }

  Status Write(const NoncontiguousBuffer& msg) { return Status{}; }
};

template <typename W, typename R>
class MockStreamReaderWriterTestProvider : public StreamReaderWriterProvider {
 public:
  using ReadFunction = std::function<Status(NoncontiguousBuffer*, int)>;

 public:
  explicit MockStreamReaderWriterTestProvider(Status&& status) : status_(std::move(status)) {}

  MockStreamReaderWriterTestProvider() : status_(Status{0, 0, "mock stream reader writer error"}) {}

  ~MockStreamReaderWriterTestProvider() override = default;

  Status Read(NoncontiguousBuffer* msg, int timeout = -1) override { return msg_reader_writer_.Read(msg, timeout); }

  Status Write(NoncontiguousBuffer&& msg) override { return msg_reader_writer_.Write(std::move(msg)); }

  Status WriteDone() override { return GetStatus(); }

  virtual Status Start() { return GetStatus(); }

  Status Finish() override { return GetStatus(); }

  void Close(Status status) override {}

  void Reset(Status status) override {}

  inline Status GetStatus() const override { return status_; }

  void SetStatus(Status status) { status_ = std::move(status); }

  MessageReaderWriter<W, R>& GetMessageReaderWriter() { return msg_reader_writer_; }

 private:
  Status status_;

  MessageReaderWriter<W, R> msg_reader_writer_;
};

template <typename W, typename R>
class MockServerStreamReaderWriterProvider : public MockStreamReaderWriterTestProvider<W, R> {
 public:
  int GetEncodeErrorCode() override { return TrpcRetCode::TRPC_STREAM_SERVER_ENCODE_ERR; }

  int GetDecodeErrorCode() override { return TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR; }

  int GetReadTimeoutErrorCode() override { return TrpcRetCode::TRPC_STREAM_SERVER_READ_TIMEOUT_ERR; }
};

template <typename W, typename R>
class MockClientStreamReaderWriterProvider : public MockStreamReaderWriterTestProvider<W, R> {
 public:
  int GetEncodeErrorCode() override { return TrpcRetCode::TRPC_STREAM_CLIENT_ENCODE_ERR; }

  int GetDecodeErrorCode() override { return TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR; }

  int GetReadTimeoutErrorCode() override { return TrpcRetCode::TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR; }
};

template <typename W, typename R>
using MockStreamReaderWriterTestProviderPtr = RefPtr<MockStreamReaderWriterTestProvider<W, R>>;
}  // namespace

namespace {
template <typename W, typename R>
struct StreamReaderWriterBuilder {
  MockStreamReaderWriterTestProviderPtr<W, R> provider;
  StreamReaderImplPtr<R> reader_impl;
  StreamWriterImplPtr<W> writer_impl;
  bool server_mode;

  explicit StreamReaderWriterBuilder(bool server_mode = false) : server_mode(server_mode) { SetUp(); }
  ~StreamReaderWriterBuilder() { TearDown(); }

  StreamReaderWriter<W, R> ReaderWriter() { return StreamReaderWriter<W, R>(reader_impl, writer_impl); }

  StreamReader<R> Reader() { return StreamReader<R>(reader_impl); }

  StreamWriter<W> Writer() { return StreamWriter<W>(writer_impl); }

 protected:
  void SetUp() {
    if (server_mode) {
      provider = MakeRefCounted<MockServerStreamReaderWriterProvider<W, R>>();
    } else {
      provider = MakeRefCounted<MockClientStreamReaderWriterProvider<W, R>>();
    }

    MessageContentCodecOptions content_codec{
        .serialization = kPbSerialization,
        // protobuf.
        .content_type = serialization::kPbMessage,
    };
    reader_impl = MakeRefCounted<StreamReaderImpl<R>>(provider, content_codec);
    writer_impl = MakeRefCounted<StreamWriterImpl<W>>(provider, content_codec);
  }

  void TearDown() {}
};
}  // namespace

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST(StreamReaderImplTest, ReadOk) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder;
  HelloReply reply{};

  builder.provider->GetMessageReaderWriter().r_msg.set_msg("Hello World!");
  ASSERT_TRUE(builder.reader_impl->Read(&reply).OK());
  ASSERT_EQ(reply.msg(), "Hello World!");
}

TEST(StreamReaderImplTest, ReadTimeout) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder;
  HelloReply reply{};

  // Reads timeout.
  builder.provider->GetMessageReaderWriter().status = Status{-1, 0, "stream read timeout"};

  auto status = builder.reader_impl->Read(&reply);
  ASSERT_FALSE(status.OK());
  ASSERT_EQ("stream read timeout", status.ErrorMessage());
}

TEST(StreamReaderImplTest, ReadBadMessageWithClientDecodeError) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder;
  HelloReply reply{};

  // Reads timeout.
  builder.provider->GetMessageReaderWriter().read_good = false;

  auto status = builder.reader_impl->Read(&reply);
  ASSERT_FALSE(status.OK());
  ASSERT_EQ(builder.reader_impl->Read(&reply).GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR);
}

TEST(StreamReaderImplTest, ReadBadMessageWithServerDecodeError) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder(true);
  HelloReply reply{};

  // Reads timeout.
  builder.provider->GetMessageReaderWriter().read_good = false;

  auto status = builder.reader_impl->Read(&reply);
  ASSERT_FALSE(status.OK());
  ASSERT_EQ(builder.reader_impl->Read(&reply).GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR);
}

TEST(StreamReaderImplTest, FinishOk) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder;
  ASSERT_TRUE(builder.reader_impl->Finish().OK());
}

TEST(StreamReaderImplTest, FinishWorkOnServerMode) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder(true);
  builder.provider->SetStatus(Status{0, 0, "work on server mode"});
  auto status = builder.reader_impl->Finish();

  ASSERT_TRUE(status.OK());
  ASSERT_EQ("work on server mode", status.ErrorMessage());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST(StreamWriterImplTest, WriteOk) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder;
  HelloRequest request{};

  // Writes ok.
  ASSERT_TRUE(builder.writer_impl->Write(request).OK());
}

TEST(StreamWriterImplTest, WriteDoneOk) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder;

  ASSERT_TRUE(builder.writer_impl->WriteDone().OK());
}

TEST(StreamWriterImplTest, WriteDoneWorkOnServerMode) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder(true);

  builder.provider->SetStatus(Status{0, 0, "work on server mode"});
  auto status = builder.writer_impl->WriteDone();

  ASSERT_TRUE(status.OK());
  ASSERT_EQ("work on server mode", status.ErrorMessage());
}

TEST(StreamWriterImplTest, FinishOk) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder;
  ASSERT_TRUE(builder.writer_impl->Finish().OK());
}

TEST(StreamWriterImplTest, FinishWorkOnServerMode) {
  using R = HelloReply;
  using W = HelloRequest;

  StreamReaderWriterBuilder<W, R> builder(true);
  builder.provider->SetStatus(Status{0, 0, "work on server mode"});
  auto status = builder.writer_impl->Finish();

  ASSERT_TRUE(status.OK());
  ASSERT_EQ("work on server mode", status.ErrorMessage());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class StreamReaderTest : public ::testing::Test {
 public:
  using R = HelloReply;
  using W = HelloRequest;

  void SetUp() override {}
  void TearDown() override {}

 protected:
  StreamReaderWriterBuilder<W, R> builder_;
};

TEST_F(StreamReaderTest, ReadOk) {
  R r;
  ASSERT_TRUE(builder_.Reader().Read(&r).OK());
}

TEST_F(StreamReaderTest, FinishOk) { ASSERT_TRUE(builder_.Reader().Finish().OK()); }

TEST_F(StreamReaderTest, GetStatusOk) { ASSERT_TRUE(builder_.Reader().GetStatus().OK()); }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class StreamWriterTest : public ::testing::Test {
 public:
  using R = HelloReply;
  using W = HelloRequest;

  void SetUp() override {}
  void TearDown() override {}

 protected:
  StreamReaderWriterBuilder<W, R> builder_;
};

TEST_F(StreamWriterTest, WriteOk) {
  W w;
  ASSERT_TRUE(builder_.Writer().Write(w).OK());
}

TEST_F(StreamWriterTest, WriteDoneOk) { ASSERT_TRUE(builder_.Writer().WriteDone().OK()); }

TEST_F(StreamWriterTest, FinishOk) { ASSERT_TRUE(builder_.Writer().Finish().OK()); }

TEST_F(StreamWriterTest, GetStatusOk) { ASSERT_TRUE(builder_.Writer().WriteDone().OK()); }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class StreamReaderWriterTest : public ::testing::Test {
 public:
  using R = HelloReply;
  using W = HelloRequest;

  void SetUp() override {}
  void TearDown() override {}

 protected:
  StreamReaderWriterBuilder<W, R> builder_;
};

TEST_F(StreamReaderWriterTest, ReadOk) {
  R r;
  ASSERT_TRUE(builder_.ReaderWriter().Read(&r).OK());
}

TEST_F(StreamReaderWriterTest, WriteOk) {
  W w;
  ASSERT_TRUE(builder_.ReaderWriter().Write(w).OK());
}

TEST_F(StreamReaderWriterTest, WriteDoneOk) { ASSERT_TRUE(builder_.ReaderWriter().WriteDone().OK()); }
TEST_F(StreamReaderWriterTest, FinishOk) { ASSERT_TRUE(builder_.ReaderWriter().Finish().OK()); }

TEST_F(StreamReaderWriterTest, GetStatusOk) { ASSERT_TRUE(builder_.ReaderWriter().GetStatus().OK()); }

TEST_F(StreamReaderWriterTest, GetReaderOk) { ASSERT_TRUE(builder_.ReaderWriter().Reader().GetStatus().OK()); }

TEST_F(StreamReaderWriterTest, GetWriterOK) { ASSERT_TRUE(builder_.ReaderWriter().Writer().GetStatus().OK()); }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
TEST(StreamReaderWriterBuilderTest, CreateOk) {
  using R = HelloReply;
  using W = HelloRequest;

  auto provider = MakeRefCounted<MockStreamReaderWriterTestProvider<W, R>>();
  MessageContentCodecOptions content_codec{
      .serialization = kPbSerialization,
      // protobuf.
      .content_type = serialization::kPbMessage,
  };

  auto reader_writer = Create<W, R>(provider, content_codec);
  ASSERT_TRUE(reader_writer.Reader().GetStatus().OK());
  ASSERT_TRUE(reader_writer.Writer().GetStatus().OK());
}

TEST(StreamReaderImplTest, TestReadRawData) {
  MockStreamReaderWriterProviderPtr stream = CreateMockStreamReaderWriterProvider();

  EXPECT_CALL(*stream, Read(::testing::_, ::testing::_)).WillOnce([](NoncontiguousBuffer* msg, int) {
    NoncontiguousBufferBuilder builder;
    builder.Append("raw data");
    *msg = std::move(builder.DestructiveGet());
    return kSuccStatus;
  });

  MessageContentCodecOptions content_codec{
      .serialization = kNoopSerialization,
      // protobuf.
      .content_type = serialization::kNoopType,
  };
  auto reader = MakeRefCounted<StreamReaderImpl<trpc::NoncontiguousBuffer>>(stream, content_codec);

  NoncontiguousBuffer buffer;
  Status status = reader->Read(&buffer);
  ASSERT_TRUE(status.OK());
  ASSERT_EQ(FlattenSlow(buffer), "raw data");
}

TEST(StreamWriterImplTest, TestWriteRawData) {
  MockStreamReaderWriterProviderPtr stream = CreateMockStreamReaderWriterProvider();

  NoncontiguousBuffer check_out;
  EXPECT_CALL(*stream, Write(::testing::_)).WillOnce([&check_out](NoncontiguousBuffer&& msg) {
    check_out = std::move(msg);
    return kSuccStatus;
  });

  MessageContentCodecOptions content_codec{
      .serialization = kNoopSerialization,
      // protobuf.
      .content_type = serialization::kNoopType,
  };
  auto writer = MakeRefCounted<StreamWriterImpl<trpc::NoncontiguousBuffer>>(stream, content_codec);

  NoncontiguousBufferBuilder builder;
  builder.Append("raw data");
  Status status = writer->Write(builder.DestructiveGet());
  ASSERT_TRUE(status.OK());
  ASSERT_EQ(FlattenSlow(check_out), "raw data");
}

}  // namespace trpc::testing
