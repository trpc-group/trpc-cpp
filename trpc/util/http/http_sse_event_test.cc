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

#include "trpc/client/http/http_sse_event.h"

#include <gtest/gtest.h>

namespace trpc::http {

class SseEventTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(SseEventTest, ToStringBasicMessage) {
  SseEvent event;
  event.data = "This is the first message.";

  std::string result = event.ToString();
  std::string expected = "data: This is the first message.\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringMultiLineMessage) {
  SseEvent event;
  event.data = "This is the second message, it\\nhas two lines.";

  std::string result = event.ToString();
  std::string expected =
      "data: This is the second message, it\\n"
      "data: has two lines.\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringWithEventType) {
  SseEvent event;
  event.event_type = "add";
  event.data = "73857293";

  std::string result = event.ToString();
  std::string expected =
      "event: add\\n"
      "data: 73857293\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringRemoveEvent) {
  SseEvent event;
  event.event_type = "remove";
  event.data = "2153";

  std::string result = event.ToString();
  std::string expected =
      "event: remove\\n"
      "data: 2153\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringCompleteEvent) {
  SseEvent event;
  event.event_type = "notification";
  event.data = "Hello World";
  event.id = "123";
  event.retry = 5000;

  std::string result = event.ToString();
  std::string expected =
      "event: notification\\n"
      "data: Hello World\\n"
      "id: 123\\n"
      "retry: 5000\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringEmptyEvent) {
  SseEvent event;

  std::string result = event.ToString();
  std::string expected = "\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringOnlyId) {
  SseEvent event;
  event.id = "msg-001";

  std::string result = event.ToString();
  std::string expected = "id: msg-001\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringOnlyRetry) {
  SseEvent event;
  event.retry = 3000;

  std::string result = event.ToString();
  std::string expected = "retry: 3000\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringMultipleDataLines) {
  SseEvent event;
  event.event_type = "multiline";
  event.data = "Line 1\\nLine 2\\nLine 3";
  event.id = "multi-001";

  std::string result = event.ToString();
  std::string expected =
      "event: multiline\\n"
      "data: Line 1\\n"
      "data: Line 2\\n"
      "data: Line 3\\n"
      "id: multi-001\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringEmptyData) {
  SseEvent event;
  event.event_type = "ping";
  event.data = "";

  std::string result = event.ToString();
  std::string expected = "event: ping\\n\\n";

  EXPECT_EQ(result, expected);
}

TEST_F(SseEventTest, ToStringSpecialCharacters) {
  SseEvent event;
  event.event_type = "special";
  event.data = "Data with: colons and\\nnewlines";
  event.id = "special-123";

  std::string result = event.ToString();
  std::string expected =
      "event: special\\n"
      "data: Data with: colons and\\n"
      "data: newlines\\n"
      "id: special-123\\n\\n";

  EXPECT_EQ(result, expected);
}

// Test sequence of events as described in the requirements
TEST_F(SseEventTest, TestSequenceOfBasicMessages) {
  std::vector<SseEvent> events;

  // First message
  SseEvent event1;
  event1.data = "This is the first message.";
  events.push_back(event1);

  // Second message (multi-line)
  SseEvent event2;
  event2.data = "This is the second message, it\\nhas two lines.";
  events.push_back(event2);

  // Third message
  SseEvent event3;
  event3.data = "This is the third message.";
  events.push_back(event3);

  std::string combined_output;
  for (const auto& event : events) {
    combined_output += event.ToString();
  }

  std::string expected =
      "data: This is the first message.\\n\\n"
      "data: This is the second message, it\\n"
      "data: has two lines.\\n\\n"
      "data: This is the third message.\\n\\n";

  EXPECT_EQ(combined_output, expected);
}

TEST_F(SseEventTest, TestSequenceOfTypedEvents) {
  std::vector<SseEvent> events;

  // Add event
  SseEvent event1;
  event1.event_type = "add";
  event1.data = "73857293";
  events.push_back(event1);

  // Remove event
  SseEvent event2;
  event2.event_type = "remove";
  event2.data = "2153";
  events.push_back(event2);

  // Another add event
  SseEvent event3;
  event3.event_type = "add";
  event3.data = "113411";
  events.push_back(event3);

  std::string combined_output;
  for (const auto& event : events) {
    combined_output += event.ToString();
  }

  std::string expected =
      "event: add\\n"
      "data: 73857293\\n\\n"
      "event: remove\\n"
      "data: 2153\\n\\n"
      "event: add\\n"
      "data: 113411\\n\\n";

  EXPECT_EQ(combined_output, expected);
}

}  // namespace trpc::http