# HTTP Server-Sent Events (SSE) Utilities

This directory contains the core utilities for implementing HTTP Server-Sent Events (SSE) support in tRPC-Cpp.

## Overview

The SSE utilities provide:
- **SseEvent**: Data structure representing a single SSE event
- **SseParser**: Parser for converting between SSE text format and SseEvent objects
- **SSE Format Compliance**: Full compliance with [W3C Server-Sent Events specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)

## Components

### 1. SseEvent

The `SseEvent` class represents a single Server-Sent Event with all standard SSE fields.

#### Header File
```cpp
#include "trpc/util/http/sse/sse_event.h"
```

#### Class Definition
```cpp
namespace trpc::http::sse {

class SseEvent {
 public:
  // Constructors
  SseEvent() = default;
  SseEvent(std::string event_type, std::string data, 
           std::optional<std::string> id = std::nullopt, 
           std::optional<uint32_t> retry = std::nullopt);
  explicit SseEvent(std::string data, 
                    std::optional<std::string> id = std::nullopt);

  // Getters
  const std::string& GetEventType() const;
  const std::string& GetData() const;
  const std::optional<std::string>& GetId() const;
  const std::optional<uint32_t>& GetRetry() const;

  // Setters
  void SetEventType(std::string event_type);
  void SetData(std::string data);
  void SetId(std::optional<std::string> id);
  void SetRetry(std::optional<uint32_t> retry);

 private:
  std::string event_type_;
  std::string data_;
  std::optional<std::string> id_;
  std::optional<uint32_t> retry_;
};

} // namespace trpc::http::sse
```

#### Field Descriptions

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `event_type_` | `std::string` | Event type identifier | `"message"`, `"update"`, `"notification"` |
| `data_` | `std::string` | Event payload data | `"Hello World"`, `"User logged in"` |
| `id_` | `std::optional<std::string>` | Event ID for reconnection | `"123"`, `"event_456"` |
| `retry_` | `std::optional<uint32_t>` | Reconnection timeout in milliseconds | `5000`, `10000` |

#### Usage Examples

```cpp
#include "trpc/util/http/sse/sse_event.h"

using namespace trpc::http::sse;

// Create a simple data event
SseEvent event1("Hello World");

// Create an event with type and data
SseEvent event2("message", "User notification");

// Create a complete event with all fields
SseEvent event3("update", "Status changed", "event_123", 5000);

// Access event properties
std::string data = event1.GetData();           // "Hello World"
std::string type = event2.GetEventType();      // "message"
auto id = event3.GetId();                      // "event_123"
auto retry = event3.GetRetry();                // 5000
```

### 2. SseParser

The `SseParser` class provides static methods for parsing and serializing SSE events.

#### Header File
```cpp
#include "trpc/util/http/sse/sse_parser.h"
```

#### Class Definition
```cpp
namespace trpc::http::sse {

class SseParser {
 public:
  // Parse SSE text to SseEvent objects
  static SseEvent ParseEvent(const std::string& text);
  static std::vector<SseEvent> ParseEvents(const std::string& text);
  
  // Serialize SseEvent objects to SSE text
  static std::string SerializeEvent(const SseEvent& event);
  static std::string SerializeEvents(const std::vector<SseEvent>& events);
  
  // Validation
  static bool IsValidSseMessage(const std::string& text);

 private:
  // Internal helper methods
  static void ParseLine(const std::string& line, SseEvent& event);
  static std::vector<std::string> SplitLines(const std::string& text);
  static std::string Trim(const std::string& str);
  static bool IsEmptyLine(const std::string& line);
};

} // namespace trpc::http::sse
```

#### Public Methods

| Method | Parameters | Return | Description |
|--------|------------|--------|-------------|
| `ParseEvent` | `const std::string& text` | `SseEvent` | Parse single SSE event from text |
| `ParseEvents` | `const std::string& text` | `std::vector<SseEvent>` | Parse multiple SSE events from text |
| `SerializeEvent` | `const SseEvent& event` | `std::string` | Convert SseEvent to SSE text format |
| `SerializeEvents` | `const std::vector<SseEvent>& events` | `std::string` | Convert multiple events to SSE text |
| `IsValidSseMessage` | `const std::string& text` | `bool` | Validate SSE message format |

#### Usage Examples

```cpp
#include "trpc/util/http/sse/sse_parser.h"

using namespace trpc::http::sse;

// Parse single event
std::string sse_text = "event: message\ndata: Hello World\n\n";
SseEvent event = SseParser::ParseEvent(sse_text);

// Parse multiple events
std::string multi_event_text = 
  "data: Event 1\n\n"
  "event: update\ndata: Event 2\n\n";
std::vector<SseEvent> events = SseParser::ParseEvents(multi_event_text);

// Serialize event to text
SseEvent my_event("notification", "User logged in", "123");
std::string serialized = SseParser::SerializeEvent(my_event);
// Result: "event: notification\nid: 123\ndata: User logged in\n\n"

// Validate SSE message
bool is_valid = SseParser::IsValidSseMessage(sse_text); // true
```

#### SSE Text Format

The parser supports the standard SSE format:

```
field: value
field: value
field: value

```

**Supported Fields:**
- `event`: Event type
- `data`: Event data (can be multi-line)
- `id`: Event identifier
- `retry`: Reconnection timeout in milliseconds
- `:` (colon only): Comment line

**Examples:**
```
# Simple data event
data: Hello World

# Event with type
event: message
data: User notification

# Complete event
event: update
id: event_123
retry: 5000
data: Status changed

# Comment
: This is a comment

# Multi-line data
data: Line 1
data: Line 2
data: Line 3
```

## Building

### Prerequisites
- Bazel build system
- C++17 or later
- tRPC-Cpp framework

### Build Commands

#### Build the SSE utilities library
```bash
bazel build //trpc/util/http/sse:http_sse
```

#### Build and run tests
```bash
# Build tests
bazel build //trpc/util/http/sse/test:sse_parser_test

# Run tests with minimal output
bazel test //trpc/util/http/sse/test:sse_parser_test

# Run tests with detailed output
bazel test //trpc/util/http/sse/test:sse_parser_test --test_output=all
```

#### Build specific targets
```bash
# Build only the header files
bazel build //trpc/util/http/sse:http_sse_headers

# Build with debug symbols
bazel build -c dbg //trpc/util/http/sse:http_sse

# Build with optimizations
bazel build -c opt //trpc/util/http/sse:http_sse
```

### Dependencies

The SSE utilities depend on:
- `//trpc/common:status` - Common status types
- Google Test framework (for tests)

### Integration

To use the SSE utilities in your project:

```cpp
// In your BUILD file
cc_library(
    name = "my_service",
    srcs = ["my_service.cc"],
    deps = [
        "//trpc/util/http/sse:http_sse",
        # ... other dependencies
    ],
)
```

## Testing

The test suite provides comprehensive coverage for:
- ✅ **Parsing**: All SSE field types and edge cases
- ✅ **Serialization**: Round-trip conversion integrity
- ✅ **Validation**: Message format validation
- ✅ **Multi-line data**: Complex data handling
- ✅ **Comments**: Comment line support

Run tests with:
```bash
bazel test //trpc/util/http/sse/test:sse_parser_test --test_output=all
```

## Notes

- **Namespace**: All components are in `trpc::http::sse` namespace
- **Thread Safety**: All methods are thread-safe (static methods with no shared state)
- **Memory Management**: Uses RAII with standard C++ containers
- **Error Handling**: Invalid input is handled gracefully with sensible defaults
- **Performance**: Optimized for typical SSE message sizes and patterns

## Related Documentation

- [Test Suite README](test/README.md) - Detailed test documentation
- [tRPC-Cpp Framework](https://github.com/trpc-group/trpc-cpp) - Main framework documentation
- [W3C SSE Specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events) - SSE standard reference
