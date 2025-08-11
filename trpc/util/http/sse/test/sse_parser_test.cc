// trpc/util/http_sse/sse_parser_test.cc
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

#include "trpc/util/http/sse/sse_parser.h"

#include <gtest/gtest.h>

namespace trpc::http::sse {

class SseParserTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test ParseEvent
TEST_F(SseParserTest, ParseEvent_SimpleData) {
  std::string sse_text = "data: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.GetData(), "Hello World");
  EXPECT_EQ(event.GetEventType(), "");
  EXPECT_FALSE(event.GetId().has_value());
  EXPECT_FALSE(event.GetRetry().has_value());
}

TEST_F(SseParserTest, ParseEvent_WithEventType) {
  std::string sse_text = "event: message\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.GetEventType(), "message");
  EXPECT_EQ(event.GetData(), "Hello World");
}

TEST_F(SseParserTest, ParseEvent_WithId) {
  std::string sse_text = "id: 123\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.GetId().value(), "123");
  EXPECT_EQ(event.GetData(), "Hello World");
}

TEST_F(SseParserTest, ParseEvent_WithRetry) {
  std::string sse_text = "retry: 5000\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.GetRetry().value(), 5000);
  EXPECT_EQ(event.GetData(), "Hello World");
}

TEST_F(SseParserTest, ParseEvent_Comment) {
  std::string sse_text = ": This is a comment\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.GetData(), ": This is a comment");
}

TEST_F(SseParserTest, ParseEvent_MultiLineData) {
  std::string sse_text = "data: Line 1\ndata: Line 2\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.GetData(), "Line 1\nLine 2");
}

TEST_F(SseParserTest, ParseEvent_CompleteEvent) {
  std::string sse_text = "event: message\nid: 123\nretry: 5000\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.GetEventType(), "message");
  EXPECT_EQ(event.GetId().value(), "123");
  EXPECT_EQ(event.GetRetry().value(), 5000);
  EXPECT_EQ(event.GetData(), "Hello World");
}

// Test ParseEvents
TEST_F(SseParserTest, ParseEvents_MultipleEvents) {
  std::string sse_text = 
    "event: message\ndata: Event 1\n\n"
    "event: update\ndata: Event 2\n\n"
    "data: Event 3\n\n";
  
  std::vector<SseEvent> events = SseParser::ParseEvents(sse_text);
  
  EXPECT_EQ(events.size(), 3);
  EXPECT_EQ(events[0].GetEventType(), "message");
  EXPECT_EQ(events[0].GetData(), "Event 1");
  EXPECT_EQ(events[1].GetEventType(), "update");
  EXPECT_EQ(events[1].GetData(), "Event 2");
  EXPECT_EQ(events[2].GetEventType(), "");
  EXPECT_EQ(events[2].GetData(), "Event 3");
}

// Test SerializeEvent
TEST_F(SseParserTest, SerializeEvent_SimpleData) {
  SseEvent event("Hello World");
  std::string serialized = SseParser::SerializeEvent(event);
  
  EXPECT_EQ(serialized, "data: Hello World\n\n");
}

TEST_F(SseParserTest, SerializeEvent_WithEventType) {
  SseEvent event(std::string("message"), std::string("Hello World"));
  std::string serialized = SseParser::SerializeEvent(event);
  
  EXPECT_EQ(serialized, "event: message\ndata: Hello World\n\n");
}

TEST_F(SseParserTest, SerializeEvent_WithId) {
  SseEvent event(std::string("Hello World"), std::optional<std::string>("123"));
  std::string serialized = SseParser::SerializeEvent(event);
  
  EXPECT_EQ(serialized, "id: 123\ndata: Hello World\n\n");
}

TEST_F(SseParserTest, SerializeEvent_WithRetry) {
  SseEvent event("", "", std::nullopt, 5000);
  std::string serialized = SseParser::SerializeEvent(event);
  
  EXPECT_EQ(serialized, "retry: 5000\n\n");
}

TEST_F(SseParserTest, SerializeEvent_Comment) {
  SseEvent event(std::string(""), std::string(": This is a comment"));
  std::string serialized = SseParser::SerializeEvent(event);
  
  EXPECT_EQ(serialized, ": This is a comment\n\n");
}

TEST_F(SseParserTest, SerializeEvent_CompleteEvent) {
  SseEvent event(std::string("message"), std::string("Hello World"), std::optional<std::string>("123"), std::optional<uint32_t>(5000));
  std::string serialized = SseParser::SerializeEvent(event);
  
  EXPECT_EQ(serialized, "event: message\nid: 123\nretry: 5000\ndata: Hello World\n\n");
}

// Test SerializeEvents
TEST_F(SseParserTest, SerializeEvents_MultipleEvents) {
  std::vector<SseEvent> events = {
    SseEvent(std::string("message"), std::string("Event 1"), std::optional<std::string>("1")),
    SseEvent(std::string("update"), std::string("Event 2"), std::optional<std::string>("2")),
    SseEvent(std::string("Event 3"))
  };
  
  std::string serialized = SseParser::SerializeEvents(events);
  
  std::string expected = 
    "event: message\nid: 1\ndata: Event 1\n\n"
    "event: update\nid: 2\ndata: Event 2\n\n"
    "data: Event 3\n\n";
  
  EXPECT_EQ(serialized, expected);
}

// Test IsValidSseMessage
TEST_F(SseParserTest, IsValidSseMessage_Valid) {
  std::string valid_sse = "data: Hello World\n\n";
  EXPECT_TRUE(SseParser::IsValidSseMessage(valid_sse));
}

TEST_F(SseParserTest, IsValidSseMessage_Invalid) {
  std::string invalid_sse = "invalid: field\n\n";
  EXPECT_FALSE(SseParser::IsValidSseMessage(invalid_sse));
}

TEST_F(SseParserTest, IsValidSseMessage_Empty) {
  std::string empty = "";
  EXPECT_TRUE(SseParser::IsValidSseMessage(empty));
}

TEST_F(SseParserTest, IsValidSseMessage_WithComments) {
  std::string sse_with_comments = ": Comment\ndata: Hello\n\n";
  EXPECT_TRUE(SseParser::IsValidSseMessage(sse_with_comments));
}

// Test round-trip serialization
TEST_F(SseParserTest, RoundTrip_SimpleEvent) {
  SseEvent original("Hello World");
  std::string serialized = SseParser::SerializeEvent(original);
  SseEvent parsed = SseParser::ParseEvent(serialized);
  
  EXPECT_EQ(parsed.GetData(), original.GetData());
  EXPECT_EQ(parsed.GetEventType(), original.GetEventType());
}

TEST_F(SseParserTest, RoundTrip_ComplexEvent) {
  SseEvent original(std::string("message"), std::string("Hello World"), std::optional<std::string>("123"), std::optional<uint32_t>(5000));
  std::string serialized = SseParser::SerializeEvent(original);
  SseEvent parsed = SseParser::ParseEvent(serialized);
  
  EXPECT_EQ(parsed.GetEventType(), original.GetEventType());
  EXPECT_EQ(parsed.GetData(), original.GetData());
  EXPECT_EQ(parsed.GetId().value(), original.GetId().value());
  EXPECT_EQ(parsed.GetRetry().value(), original.GetRetry().value());
}

}  // namespace trpc::http::sse