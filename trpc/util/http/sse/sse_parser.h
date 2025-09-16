// trpc/util/http/sse/sse_parser.h
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

#pragma once

#include <string>
#include <vector>
#include <optional>

#include "trpc/util/http/sse/sse_event.h"

namespace trpc::http::sse {

/// @brief Parser for Server-Sent Events (SSE) messages.
/// @note Parses SSE text format into SseEvent objects.
class SseParser {
 public:
  /// @brief Parse a single SSE message from text
  /// @param text The SSE text to parse
  /// @return Parsed SseEvent object
  static SseEvent ParseEvent(const std::string& text);

  /// @brief Parse multiple SSE events from text (separated by double newlines)
  /// @param text The SSE text to parse
  /// @return Vector of parsed SseEvent objects
  static std::vector<SseEvent> ParseEvents(const std::string& text);

  /// @brief Check if a string is a valid SSE message
  /// @param text The text to validate
  /// @return true if valid SSE format, false otherwise
  static bool IsValidSseMessage(const std::string& text);

 private:
  /// @brief Parse a single line of SSE data
  /// @param line The line to parse
  /// @param event The event to populate
  static void ParseLine(const std::string& line, SseEvent& event);

  /// @brief Split text into lines
  /// @param text The text to split
  /// @return Vector of lines
  static std::vector<std::string> SplitLines(const std::string& text);

  /// @brief Trim whitespace from a string
  /// @param str The string to trim
  /// @return Trimmed string
  static std::string Trim(const std::string& str);

  /// @brief Check if a line is empty or contains only whitespace
  /// @param line The line to check
  /// @return true if empty, false otherwise
  static bool IsEmptyLine(const std::string& line);
};

}  // namespace trpc::http::sse