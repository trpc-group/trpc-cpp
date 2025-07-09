#include <optional>
#include <sstream>
#include <string>
namespace trpc::http {

/// @brief SSE event structure
struct SseEvent {
  std::string event_type;    // event field
  std::string data;          // data field
  std::string id;            // id field
  std::optional<int> retry;  // retry field (milliseconds)

  /// @brief Serialize SSE event to string format
  std::string ToString() const {
    std::string result;

    if (!event_type.empty()) {
      result += "event: " + event_type + "\n";
    }

    if (!data.empty()) {
      // Handle multi-line data
      std::istringstream iss(data);
      std::string line;
      while (std::getline(iss, line)) {
        result += "data: " + line + "\n";
      }
    }

    if (!id.empty()) {
      result += "id: " + id + "\n";
    }

    if (retry.has_value()) {
      result += "retry: " + std::to_string(retry.value()) + "\n";
    }

    result += "\n";  // End with double newline
    return result;
  }
};

}  // namespace trpc::http