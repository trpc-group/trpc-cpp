// trpc/util/http/sse/sse_event.h
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

#include <optional>
#include <sstream>
#include <string>
#include <cstdint>

namespace trpc::http::sse {

/// @brief SSE event structure
struct SseEvent {
  std::string event_type;                    ///< Event type
  std::string data;                          ///< Event data
  std::optional<std::string> id;             ///< Event ID (optional) 
  std::optional<uint32_t> retry;             ///< Retry timeout in milliseconds (optional)

  /// @brief Serialize SSE event to string format
  std::string ToString() const {
    std::string result;

    if (!event_type.empty()) {
      result += "event: " + event_type + "\n";
    }

    if (!data.empty()) {
      // Handle multi-line data - split on both literal "\n" sequences and actual newlines
      size_t pos = 0;
      size_t prev_pos = 0;
      
      // First, handle literal "\n" sequences
      while ((pos = data.find("\\n", prev_pos)) != std::string::npos) {
        result += "data: " + data.substr(prev_pos, pos - prev_pos) + "\n";
        prev_pos = pos + 2; // Skip over "\n"
      }
      
      // Then handle any remaining actual newline characters
      std::string remaining = data.substr(prev_pos);
      if (!remaining.empty()) {
        size_t nl_pos = 0;
        size_t nl_prev_pos = 0;
        while ((nl_pos = remaining.find('\n', nl_prev_pos)) != std::string::npos) {
          result += "data: " + remaining.substr(nl_prev_pos, nl_pos - nl_prev_pos) + "\n";
          nl_prev_pos = nl_pos + 1; // Skip over the newline
        }
        // Add the last part
        if (nl_prev_pos < remaining.length()) {
          result += "data: " + remaining.substr(nl_prev_pos) + "\n";
        }
      }
    }

    if (id.has_value() && !id.value().empty()) {
      result += "id: " + id.value() + "\n";
    }

    if (retry.has_value()) {
      result += "retry: " + std::to_string(retry.value()) + "\n";
    }

    result += "\n";  // End with double newline
    return result;
  }
};

}  // namespace trpc::http::sse