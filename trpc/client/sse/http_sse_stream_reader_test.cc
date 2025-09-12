#include "trpc/client/sse/http_sse_stream_reader.h"

#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/stream/testing/mock_stream_provider.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/stream/stream_provider.h"

namespace trpc::testing {

using namespace trpc::http::sse;

class HttpSseStreamReaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_provider_ = CreateMockStreamReaderWriterProvider();
    reader_ = std::make_unique<HttpSseStreamReader>(mock_provider_);
  }

  void TearDown() override {}

  MockStreamReaderWriterProviderPtr mock_provider_;
  std::unique_ptr<HttpSseStreamReader> reader_;
};

TEST_F(HttpSseStreamReaderTest, DefaultConstructor) {
  HttpSseStreamReader reader;
  EXPECT_FALSE(reader.IsValid());
}

TEST_F(HttpSseStreamReaderTest, ConstructorWithStreamProvider) {
  EXPECT_TRUE(reader_->IsValid());
}

TEST_F(HttpSseStreamReaderTest, MoveConstructor) {
  HttpSseStreamReader moved_reader = std::move(*reader_);
  EXPECT_TRUE(moved_reader.IsValid());
}

TEST_F(HttpSseStreamReaderTest, MoveAssignment) {
  HttpSseStreamReader moved_reader;
  moved_reader = std::move(*reader_);
  EXPECT_TRUE(moved_reader.IsValid());
}

TEST_F(HttpSseStreamReaderTest, Read) {
  // Mock the Read method to return success
  EXPECT_CALL(*mock_provider_, Read(::testing::NotNull(), ::testing::Ge(-1)))
      .WillOnce(::testing::Return(Status{0, 0, "OK"}));
  
  SseEvent event;
  Status status = reader_->Read(&event);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamReaderTest, ReadRaw) {
  // Mock the Read method to return success
  EXPECT_CALL(*mock_provider_, Read(::testing::NotNull(), ::testing::Ge(-1)))
      .WillOnce(::testing::Return(Status{0, 0, "OK"}));
  
  NoncontiguousBuffer data;
  Status status = reader_->ReadRaw(&data, 100);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamReaderTest, ReadAll) {
  // Mock the Read method to return success
  EXPECT_CALL(*mock_provider_, Read(::testing::NotNull(), ::testing::Ge(-1)))
      .WillOnce(::testing::Return(Status{0, 0, "OK"}));
  
  NoncontiguousBuffer data;
  Status status = reader_->ReadAll(&data);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamReaderTest, Finish) {
  // Mock the Finish method to return success
  EXPECT_CALL(*mock_provider_, Finish())
      .WillOnce(::testing::Return(Status{0, 0, "OK"}));
  
  Status status = reader_->Finish();
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamReaderTest, GetStatus) {
  // Mock the GetStatus method to return success
  EXPECT_CALL(*mock_provider_, GetStatus())
      .WillOnce(::testing::Return(Status{0, 0, "OK"}));
  
  Status status = reader_->GetStatus();
  EXPECT_TRUE(status.OK());
}

// Test the new StartStreaming method
TEST_F(HttpSseStreamReaderTest, StartStreamingWithValidCallback) {
  // Test with a simple callback
  auto callback = [](const SseEvent& event) {
    // Do nothing in the callback for this test
  };
  
  // StartStreaming will fail with our mock provider since it's not an HttpClientStream
  // We expect it to return an error status rather than throwing an exception
  Status status = reader_->StartStreaming(callback, 1000); // 1 second timeout
  // We expect this to fail since our mock_provider_ is not an HttpClientStream
  EXPECT_FALSE(status.OK());
  // The specific error code may vary, so we just check that it's an error
}

// Test StartStreaming with null callback
TEST_F(HttpSseStreamReaderTest, StartStreamingWithNullCallback) {
  // StartStreaming should return an error status when callback is null
  Status status = reader_->StartStreaming(nullptr, 1000);
  EXPECT_FALSE(status.OK());
}

// Test StartStreaming with zero timeout
TEST_F(HttpSseStreamReaderTest, StartStreamingWithZeroTimeout) {
  auto callback = [](const SseEvent& event) {
    // Do nothing in the callback for this test
  };
  
  Status status = reader_->StartStreaming(callback, 0);
  EXPECT_FALSE(status.OK());
}

// Test StartStreaming with negative timeout
TEST_F(HttpSseStreamReaderTest, StartStreamingWithNegativeTimeout) {
  auto callback = [](const SseEvent& event) {
    // Do nothing in the callback for this test
  };
  
  Status status = reader_->StartStreaming(callback, -1);
  EXPECT_FALSE(status.OK());
}

// Test ReadHeaders method
TEST_F(HttpSseStreamReaderTest, ReadHeaders) {
  int code = 0;
  trpc::http::HttpHeader headers;
  
  // ReadHeaders will fail with our mock provider since it's not an HttpClientStream
  // It should return an error status rather than throwing an exception
  Status status = reader_->ReadHeaders(code, headers);
  // We expect this to fail since our mock_provider_ is not an HttpClientStream
  EXPECT_FALSE(status.OK());
}

// Test SseEventCallback type
TEST_F(HttpSseStreamReaderTest, SseEventCallbackType) {
  // Test that SseEventCallback type is correctly defined by creating one
  HttpSseStreamReader::SseEventCallback callback = [](const SseEvent& event) {
    // Do nothing in the callback for this test
  };
  EXPECT_TRUE(true); // If compilation succeeds, the type is correctly defined
}

// Test IsValid method
TEST_F(HttpSseStreamReaderTest, IsValid) {
  // Valid reader should return true
  EXPECT_TRUE(reader_->IsValid());
  
  // Invalid reader should return false
  HttpSseStreamReader invalid_reader;
  EXPECT_FALSE(invalid_reader.IsValid());
}

}  // namespace trpc::testing