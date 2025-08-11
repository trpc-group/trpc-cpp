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

#include "trpc/codec/http_sse/http_sse_codec.h"

#include <gtest/gtest.h>

#include "trpc/client/client_context.h"
#include "trpc/server/server_context.h"
#include "trpc/util/http/sse/sse_parser.h"

namespace trpc::test {

class HttpSseCodecTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test HttpSseRequestProtocol
TEST_F(HttpSseCodecTest, HttpSseRequestProtocol_GetSseEvent) {
  HttpSseRequestProtocol protocol;
  
  // Test with empty request
  auto event = protocol.GetSseEvent();
  EXPECT_FALSE(event.has_value());
  
  // Test with valid SSE data
  http::sse::SseEvent test_event("message", "Hello World", "123");
  protocol.SetSseEvent(test_event);
  
  auto parsed_event = protocol.GetSseEvent();
  EXPECT_TRUE(parsed_event.has_value());
  EXPECT_EQ(parsed_event->GetEventType(), "message");
  EXPECT_EQ(parsed_event->GetData(), "Hello World");
  EXPECT_EQ(parsed_event->GetId().value(), "123");
}

TEST_F(HttpSseCodecTest, HttpSseRequestProtocol_SetSseEvent) {
  HttpSseRequestProtocol protocol;
  
  http::sse::SseEvent test_event("update", "Status changed", "456", 5000);
  protocol.SetSseEvent(test_event);
  
  EXPECT_TRUE(protocol.request != nullptr);
  // Note: GetContentType() is not available, we check the content instead
  EXPECT_FALSE(protocol.request->GetContent().empty());
  
  auto parsed_event = protocol.GetSseEvent();
  EXPECT_TRUE(parsed_event.has_value());
  EXPECT_EQ(parsed_event->GetEventType(), "update");
  EXPECT_EQ(parsed_event->GetData(), "Status changed");
  EXPECT_EQ(parsed_event->GetId().value(), "456");
  EXPECT_EQ(parsed_event->GetRetry().value(), 5000);
}

// Test HttpSseResponseProtocol
TEST_F(HttpSseCodecTest, HttpSseResponseProtocol_GetSseEvent) {
  HttpSseResponseProtocol protocol;
  
  // Test with empty response
  auto event = protocol.GetSseEvent();
  EXPECT_FALSE(event.has_value());
  
  // Test with valid SSE data
  http::sse::SseEvent test_event("notification", "User logged in");
  protocol.SetSseEvent(test_event);
  
  auto parsed_event = protocol.GetSseEvent();
  EXPECT_TRUE(parsed_event.has_value());
  EXPECT_EQ(parsed_event->GetEventType(), "notification");
  EXPECT_EQ(parsed_event->GetData(), "User logged in");
}

TEST_F(HttpSseCodecTest, HttpSseResponseProtocol_SetSseEvent) {
  HttpSseResponseProtocol protocol;
  
  http::sse::SseEvent test_event("message", "Hello World");
  protocol.SetSseEvent(test_event);
  
  // Note: response is not a pointer, it's a direct member
  EXPECT_FALSE(protocol.response.GetContent().empty());
  
  auto parsed_event = protocol.GetSseEvent();
  EXPECT_TRUE(parsed_event.has_value());
  EXPECT_EQ(parsed_event->GetEventType(), "message");
  EXPECT_EQ(parsed_event->GetData(), "Hello World");
}

TEST_F(HttpSseCodecTest, HttpSseResponseProtocol_SetSseEvents) {
  HttpSseResponseProtocol protocol;
  
  std::vector<http::sse::SseEvent> events = {
    http::sse::SseEvent("message", "Event 1", "1"),
    http::sse::SseEvent("update", "Event 2", "2"),
    http::sse::SseEvent("", "Event 3")
  };
  
  protocol.SetSseEvents(events);
  
  // Note: response is not a pointer, it's a direct member
  EXPECT_FALSE(protocol.response.GetContent().empty());
  
  // Verify the serialized content contains all events
  std::string content = protocol.response.GetContent();
  EXPECT_FALSE(content.empty());
  EXPECT_NE(content.find("Event 1"), std::string::npos);
  EXPECT_NE(content.find("Event 2"), std::string::npos);
  EXPECT_NE(content.find("Event 3"), std::string::npos);
}

// Test HttpSseClientCodec
TEST_F(HttpSseCodecTest, HttpSseClientCodec_CreateRequestPtr) {
  HttpSseClientCodec codec;
  
  auto request_ptr = codec.CreateRequestPtr();
  EXPECT_TRUE(request_ptr != nullptr);
  
  auto sse_request = std::dynamic_pointer_cast<HttpSseRequestProtocol>(request_ptr);
  EXPECT_TRUE(sse_request != nullptr);
}

TEST_F(HttpSseCodecTest, HttpSseClientCodec_CreateResponsePtr) {
  HttpSseClientCodec codec;
  
  auto response_ptr = codec.CreateResponsePtr();
  EXPECT_TRUE(response_ptr != nullptr);
  
  auto sse_response = std::dynamic_pointer_cast<HttpSseResponseProtocol>(response_ptr);
  EXPECT_TRUE(sse_response != nullptr);
}

TEST_F(HttpSseCodecTest, HttpSseClientCodec_FillRequest) {
  HttpSseClientCodec codec;
  auto request_ptr = codec.CreateRequestPtr();
  auto sse_request = std::dynamic_pointer_cast<HttpSseRequestProtocol>(request_ptr);
  
  http::sse::SseEvent test_event("message", "Hello World", "123");
  
  bool result = codec.FillRequest(nullptr, request_ptr, &test_event);
  EXPECT_TRUE(result);
  
  auto parsed_event = sse_request->GetSseEvent();
  EXPECT_TRUE(parsed_event.has_value());
  EXPECT_EQ(parsed_event->GetEventType(), "message");
  EXPECT_EQ(parsed_event->GetData(), "Hello World");
  EXPECT_EQ(parsed_event->GetId().value(), "123");
  
  // Verify SSE headers are set
  EXPECT_EQ(sse_request->request->GetHeader("Accept"), "text/event-stream");
  EXPECT_EQ(sse_request->request->GetHeader("Cache-Control"), "no-cache");
  EXPECT_EQ(sse_request->request->GetHeader("Connection"), "keep-alive");
}

TEST_F(HttpSseCodecTest, HttpSseClientCodec_FillResponse) {
  HttpSseClientCodec codec;
  auto response_ptr = codec.CreateResponsePtr();
  auto sse_response = std::dynamic_pointer_cast<HttpSseResponseProtocol>(response_ptr);
  
  // Set up response with SSE data
  http::sse::SseEvent test_event("message", "Hello World", "123");
  sse_response->SetSseEvent(test_event);
  
  http::sse::SseEvent received_event;
  bool result = codec.FillResponse(nullptr, response_ptr, &received_event);
  EXPECT_TRUE(result);
  
  EXPECT_EQ(received_event.GetEventType(), "message");
  EXPECT_EQ(received_event.GetData(), "Hello World");
  EXPECT_EQ(received_event.GetId().value(), "123");
}

// Test HttpSseServerCodec
TEST_F(HttpSseCodecTest, HttpSseServerCodec_CreateRequestObject) {
  HttpSseServerCodec codec;
  
  auto request_ptr = codec.CreateRequestObject();
  EXPECT_TRUE(request_ptr != nullptr);
  
  auto sse_request = std::dynamic_pointer_cast<HttpSseRequestProtocol>(request_ptr);
  EXPECT_TRUE(sse_request != nullptr);
}

TEST_F(HttpSseCodecTest, HttpSseServerCodec_CreateResponseObject) {
  HttpSseServerCodec codec;
  
  auto response_ptr = codec.CreateResponseObject();
  EXPECT_TRUE(response_ptr != nullptr);
  
  auto sse_response = std::dynamic_pointer_cast<HttpSseResponseProtocol>(response_ptr);
  EXPECT_TRUE(sse_response != nullptr);
}

TEST_F(HttpSseCodecTest, HttpSseServerCodec_IsValidSseRequest) {
  HttpSseServerCodec codec;
  
  // Create a valid SSE request
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  request->SetHeader("Accept", "text/event-stream");
  
  bool is_valid = codec.IsValidSseRequest(request.get());
  EXPECT_TRUE(is_valid);
  
  // Test invalid request (wrong method)
  request->SetMethod("POST");
  is_valid = codec.IsValidSseRequest(request.get());
  EXPECT_FALSE(is_valid);
  
  // Test invalid request (wrong Accept header)
  request->SetMethod("GET");
  request->SetHeader("Accept", "application/json");
  is_valid = codec.IsValidSseRequest(request.get());
  EXPECT_FALSE(is_valid);
}

// Test codec names
TEST_F(HttpSseCodecTest, CodecNames) {
  HttpSseClientCodec client_codec;
  HttpSseServerCodec server_codec;
  
  EXPECT_EQ(client_codec.Name(), "http_sse");
  EXPECT_EQ(server_codec.Name(), "http_sse");
}

}  // namespace trpc::test
