// trpc/util/http/sse/sse_parser.cc
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

#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdint>

namespace trpc::http::sse {

SseEvent SseParser::ParseEvent(const std::string& text) {
  SseEvent event;
  auto lines = SplitLines(text);
  
  for (const auto& line : lines) {
    if (IsEmptyLine(line)) {
      continue;
    }
    ParseLine(line, event);
  }
  
  return event;
}

std::vector<SseEvent> SseParser::ParseEvents(const std::string& text) {
  std::vector<SseEvent> events;
  auto lines = SplitLines(text);
  
  SseEvent current_event;
  bool has_data = false;
  
  for (const auto& line : lines) {
    if (IsEmptyLine(line)) {
      // Empty line indicates end of event
      if (has_data) {
        events.push_back(current_event);
        current_event = SseEvent();
        has_data = false;
      }
      continue;
    }
    
    ParseLine(line, current_event);
    has_data = true;
  }
  
  // Don't forget the last event if there's no trailing empty line
  if (has_data) {
    events.push_back(current_event);
  }
  
  return events;
}



bool SseParser::IsValidSseMessage(const std::string& text) {
  auto lines = SplitLines(text);
  
  for (const auto& line : lines) {
    if (IsEmptyLine(line)) {
      continue;
    }
    
    // Check if line starts with valid SSE field
    std::string trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }
    
    // Valid SSE fields: event:, id:, retry:, data:, or comment (starts with :)
    if (trimmed.substr(0, 6) == "event:" ||
        trimmed.substr(0, 3) == "id:" ||
        trimmed.substr(0, 6) == "retry:" ||
        trimmed.substr(0, 5) == "data:" ||
        trimmed[0] == ':') {
      continue;
    }
    
    return false;
  }
  
  return true;
}

void SseParser::ParseLine(const std::string& line, SseEvent& event) {
  std::string trimmed = Trim(line);
  if (trimmed.empty()) {
    return;
  }
  
  // Handle comment lines
  if (trimmed[0] == ':') {
    event.data = trimmed;
    return;
  }
  
  // Parse field:value format
  size_t colon_pos = trimmed.find(':');
  if (colon_pos == std::string::npos) {
    return;
  }
  
  std::string field = Trim(trimmed.substr(0, colon_pos));
  std::string value;
  
  // Handle value with optional leading space
  if (colon_pos + 1 < trimmed.length()) {
    if (trimmed[colon_pos + 1] == ' ') {
      value = trimmed.substr(colon_pos + 2);
    } else {
      value = trimmed.substr(colon_pos + 1);
    }
  }
  
  if (field == "event") {
    event.event_type = value;
  } else if (field == "id") {
    event.id = value;
  } else if (field == "retry") {
    try {
      uint32_t retry_ms = std::stoul(value);
      event.retry = retry_ms;
    } catch (const std::exception&) {
      // Invalid retry value, ignore
    }
  } else if (field == "data") {
    // Append to existing data with newline
    if (!event.data.empty()) {
      event.data += "\n";
    }
    event.data += value;
  }
}

std::vector<std::string> SseParser::SplitLines(const std::string& text) {
  std::vector<std::string> lines;
  std::istringstream iss(text);
  std::string line;
  
  while (std::getline(iss, line)) {
    // Don't remove carriage return - preserve original line endings
    lines.push_back(line);
  }
  
  return lines;
}

std::string SseParser::Trim(const std::string& str) {
  size_t start = 0;
  size_t end = str.length();
  
  while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
    ++start;
  }
  
  while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
    --end;
  }
  
  return str.substr(start, end - start);
}

bool SseParser::IsEmptyLine(const std::string& line) {
  return line.empty() || std::all_of(line.begin(), line.end(), 
                                    [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}

}  // namespace trpc::http::sse