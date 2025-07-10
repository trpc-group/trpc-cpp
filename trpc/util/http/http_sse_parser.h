#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "trpc/client/http/http_sse_event.h"

namespace trpc::http {

/// @brief SSE parser for parsing text messages to SseEvent objects
class SseParser {
 public:
  /// @brief Parse SSE text stream into SseEvent objects
  /// @param text SSE formatted text stream
  /// @return Vector of parsed SseEvent objects
  static std::vector<SseEvent> Parse(const std::string& text) {
    std::vector<SseEvent> events;
    std::istringstream stream(text);
    std::string line;

    SseEvent current_event;
    std::vector<std::string> data_lines;

    while (std::getline(stream, line)) {
      // Remove \\r if present (handle \\r\\n line endings)
      if (!line.empty() && line.back() == '\\r') {
        line.pop_back();
      }

      // Empty line indicates end of event
      if (line.empty()) {
        if (!data_lines.empty() || !current_event.event_type.empty() || !current_event.id.empty() ||
            current_event.retry.has_value()) {
          // Join data lines with newlines
          if (!data_lines.empty()) {
            current_event.data = JoinDataLines(data_lines);
          }

          // Set default event type if not specified
          if (current_event.event_type.empty()) {
            current_event.event_type = "message";
          }

          events.push_back(current_event);

          // Reset for next event
          current_event = SseEvent{};
          data_lines.clear();
        }
        continue;
      }

      // Parse field lines
      auto colon_pos = line.find(':');
      if (colon_pos == std::string::npos) {
        continue;  // Skip malformed lines
      }

      std::string field = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);

      // Remove leading space from value if present
      if (!value.empty() && value[0] == ' ') {
        value = value.substr(1);
      }

      if (field == "data") {
        data_lines.push_back(value);
      } else if (field == "event") {
        current_event.event_type = value;
      } else if (field == "id") {
        current_event.id = value;
      } else if (field == "retry") {
        try {
          current_event.retry = std::stoi(value);
        } catch (const std::exception&) {
          // Ignore invalid retry values
        }
      }
      // Ignore unknown fields as per SSE specification
    }

    // Handle case where stream doesn't end with empty line
    if (!data_lines.empty() || !current_event.event_type.empty() || !current_event.id.empty() ||
        current_event.retry.has_value()) {
      if (!data_lines.empty()) {
        current_event.data = JoinDataLines(data_lines);
      }
      if (current_event.event_type.empty()) {
        current_event.event_type = "message";
      }
      events.push_back(current_event);
    }

    return events;
  }

  /// @brief Serialize SseEvent object to text format
  /// @param event SseEvent object to serialize
  /// @return SSE formatted text string
  static std::string Serialize(const SseEvent& event) { return event.ToString(); }

  /// @brief Serialize multiple SseEvent objects to text format
  /// @param events Vector of SseEvent objects to serialize
  /// @return SSE formatted text stream
  static std::string SerializeMultiple(const std::vector<SseEvent>& events) {
    std::string result;
    for (const auto& event : events) {
      result += event.ToString();
    }
    return result;
  }

 private:
  /// @brief Join data lines with newlines
  static std::string JoinDataLines(const std::vector<std::string>& lines) {
    if (lines.empty()) return "";
    if (lines.size() == 1) return lines[0];

    std::string result = lines[0];
    for (size_t i = 1; i < lines.size(); ++i) {
      result += "\\n" + lines[i];
    }
    return result;
  }
};

}  // namespace trpc::http