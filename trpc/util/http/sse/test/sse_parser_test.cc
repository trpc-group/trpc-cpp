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
  
  EXPECT_EQ(event.data, "Hello World");
  EXPECT_EQ(event.event_type, "");
  EXPECT_FALSE(event.id.has_value());
  EXPECT_FALSE(event.retry.has_value());
}

TEST_F(SseParserTest, ParseEvent_WithEventType) {
  std::string sse_text = "event: message\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.event_type, "message");
  EXPECT_EQ(event.data, "Hello World");
}

TEST_F(SseParserTest, ParseEvent_WithId) {
  std::string sse_text = "id: 123\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.id.value(), "123");
  EXPECT_EQ(event.data, "Hello World");
}

TEST_F(SseParserTest, ParseEvent_WithRetry) {
  std::string sse_text = "retry: 5000\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.retry.value(), 5000);
  EXPECT_EQ(event.data, "Hello World");
}

TEST_F(SseParserTest, ParseEvent_Comment) {
  std::string sse_text = ": This is a comment\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.data, ": This is a comment");
}

TEST_F(SseParserTest, ParseEvent_MultiLineData) {
  std::string sse_text = "data: Line 1\ndata: Line 2\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.data, "Line 1\nLine 2");
}

TEST_F(SseParserTest, ParseEvent_CompleteEvent) {
  std::string sse_text = "event: message\nid: 123\nretry: 5000\ndata: Hello World\n\n";
  SseEvent event = SseParser::ParseEvent(sse_text);
  
  EXPECT_EQ(event.event_type, "message");
  EXPECT_EQ(event.id.value(), "123");
  EXPECT_EQ(event.retry.value(), 5000);
  EXPECT_EQ(event.data, "Hello World");
}

// Test ParseEvents
TEST_F(SseParserTest, ParseEvents_MultipleEvents) {
  std::string sse_text = 
    "event: message\ndata: Event 1\n\n"
    "event: update\ndata: Event 2\n\n"
    "data: Event 3\n\n";
  
  std::vector<SseEvent> events = SseParser::ParseEvents(sse_text);
  
  EXPECT_EQ(events.size(), 3);
  EXPECT_EQ(events[0].event_type, "message");
  EXPECT_EQ(events[0].data, "Event 1");
  EXPECT_EQ(events[1].event_type, "update");
  EXPECT_EQ(events[1].data, "Event 2");
  EXPECT_EQ(events[2].event_type, "");
  EXPECT_EQ(events[2].data, "Event 3");
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



}  // namespace trpc::http::sse