# HTTP Server-Sent Events (SSE) Utilities

This directory contains the core utilities for implementing HTTP Server-Sent Events (SSE) support in tRPC-Cpp.

## Overview

The SSE utilities provide:
- **SseEvent**: Data structure representing a single SSE event with built-in serialization
- **SseParser**: Parser for converting SSE text format to SseEvent objects
- **SSE Format Compliance**: Full compliance with [W3C Server-Sent Events specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)

## Components

### 1. SseEvent

The `SseEvent` struct represents a single Server-Sent Event with all standard SSE fields and built-in serialization.

#### Header File
```cpp
#include "trpc/util/http/sse/sse_event.h"
```

#### Struct Definition
```cpp
namespace trpc::http::sse {

struct SseEvent {
  std::string event_type;                    ///< Event type (optional)
  std::string data;                          ///< Event data
  std::optional<std::string> id;             ///< Event ID (optional)
  std::optional<uint32_t> retry;             ///< Retry timeout in milliseconds (optional)

  /// @brief Serialize SSE event to string format
  std::string ToString() const;
};

} // namespace trpc::http::sse
```

#### Field Descriptions

| Field        | Type                         | Description                          | Example                                   |
| ------------ | ---------------------------- | ------------------------------------ | ----------------------------------------- |
| `event_type` | `std::string`                | Event type identifier                | `"message"`, `"update"`, `"notification"` |
| `data`       | `std::string`                | Event payload data                   | `"Hello World"`, `"User logged in"`       |
| `id`         | `std::optional<std::string>` | Event ID for reconnection            | `"123"`, `"event_456"`                    |
| `retry`      | `std::optional<uint32_t>`    | Reconnection timeout in milliseconds | `5000`, `10000`                           |


**SseStruct Features:**
- Every field line must end with `\n`
- `\n\n` marks the end of a message
- Each line of data must start with a field name such as "data:" or "event:", followed by a space after the colon.
- The default event type is "message".
#### Serialization

The `ToString()` method serializes the event to standard SSE text format:

```cpp
SseEvent event{};
event.event_type = "message";
event.data = "Hello World";
event.id = "123";
event.retry = 5000;

std::string serialized = event.ToString();
// Result: "event: message\ndata: Hello World\nid: 123\nretry: 5000\n\n"
```

**Serialization Features:**
- Handles multi-line data with literal `\n` sequences
- Omits empty optional fields
- Follows SSE specification field order
- Ends with double newline as required by SSE standard

#### Usage Examples

```cpp
#include "trpc/util/http/sse/sse_event.h"

using namespace trpc::http::sse;

// Create a simple data event
SseEvent event1{};
event1.data = "Hello World";

// Create an event with type and data
SseEvent event2{};
event2.event_type = "message";
event2.data = "User notification";

// Create a complete event with all fields
SseEvent event3{};
event3.event_type = "update";
event3.data = "Status changed";
event3.id = "event_123";
event3.retry = 5000;

// Access event properties
std::string data = event1.data;              // "Hello World"
std::string type = event2.event_type;        // "message"
auto id = event3.id;                         // "event_123"
auto retry = event3.retry;                   // 5000

// Serialize to SSE text
std::string sse_text = event3.ToString();
```

### 2. SseParser

The `SseParser` class provides static methods for parsing SSE text into `SseEvent` objects.

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

| Method              | Parameters                | Return                  | Description                         |
| ------------------- | ------------------------- | ----------------------- | ----------------------------------- |
| `ParseEvent`        | `const std::string& text` | `SseEvent`              | Parse single SSE event from text    |
| `ParseEvents`       | `const std::string& text` | `std::vector<SseEvent>` | Parse multiple SSE events from text |
| `IsValidSseMessage` | `const std::string& text` | `bool`                  | Validate SSE message format         |

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

// Validate SSE message
bool is_valid = SseParser::IsValidSseMessage(sse_text); // true

// Serialize using SseEvent's ToString method
std::string serialized = event.ToString();
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

### Build Structure

The SSE utilities are split into two libraries:

- **`http_sse`**: Header-only library containing `SseEvent` struct
- **`http_sse_parser`**: Implementation library containing `SseParser` class

### Build Commands

#### Build the SSE utilities libraries
```bash
# Build both libraries
bazel build //trpc/util/http/sse:http_sse //trpc/util/http/sse:http_sse_parser

# Build only headers (for header-only usage)
bazel build //trpc/util/http/sse:http_sse

# Build parser implementation
bazel build //trpc/util/http/sse:http_sse_parser
```

#### Build and run tests
```bash
# Build all tests
bazel build //trpc/util/http/sse/test/...

# Run all SSE tests
bazel test //trpc/util/http/sse/test/...

# Run specific test
bazel test //trpc/util/http/sse/test:sse_event_test
bazel test //trpc/util/http/sse/test:sse_parser_test
```

#### Build with different configurations
```bash
# Build with debug symbols
bazel build -c dbg //trpc/util/http/sse:http_sse_parser

# Build with optimizations
bazel build -c opt //trpc/util/http/sse:http_sse_parser
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
        "//trpc/util/http/sse:http_sse",           // For SseEvent only
        "//trpc/util/http/sse:http_sse_parser",    // For SseParser functionality
        # ... other dependencies
    ],
)
```

## Testing

The test suite provides comprehensive coverage for:
- ✅ **SseEvent**: Struct creation, field access, serialization
- ✅ **SseParser**: All SSE field types and edge cases
- ✅ **Validation**: Message format validation
- ✅ **Multi-line data**: Complex data handling
- ✅ **Comments**: Comment line support

Run tests with:
```bash
# Run all SSE tests
bazel test //trpc/util/http/sse/test/...

# Run with detailed output
bazel test //trpc/util/http/sse/test/... --test_output=all
```

## Architecture

### Design Principles

- **Single Responsibility**: `SseEvent` handles serialization, `SseParser` handles parsing
- **No Duplication**: Single serialization method (`SseEvent::ToString()`)
- **Simple API**: Direct struct member access, no getters/setters
- **SSE Compliance**: Full compliance with W3C SSE specification

### Serialization Strategy

```cpp
// Serialization is handled by SseEvent::ToString()
SseEvent event{};
event.data = "Hello World";
std::string sse_text = event.ToString();

// Parsing is handled by SseParser
SseEvent parsed = SseParser::ParseEvent(sse_text);
```

## Notes

- **Namespace**: All components are in `trpc::http::sse` namespace
- **Thread Safety**: All methods are thread-safe (static methods with no shared state)
- **Memory Management**: Uses RAII with standard C++ containers
- **Error Handling**: Invalid input is handled gracefully with sensible defaults
- **Performance**: Optimized for typical SSE message sizes and patterns
- **C++17 Features**: Uses `std::optional` for optional fields

## Related Documentation

- [Test Suite README](test/README.md) - Detailed test documentation
- [tRPC-Cpp Framework](https://github.com/trpc-group/trpc-cpp) - Main framework documentation
- [W3C SSE Specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events) - SSE standard reference
