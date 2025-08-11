# SSE Parser Test Suite

This directory contains comprehensive tests for the Server-Sent Events (SSE) parser implementation in tRPC-Cpp.

## Overview

The test suite validates the `SseParser` class functionality, which handles:
- Parsing SSE text format into `SseEvent` objects
- Serializing `SseEvent` objects back to SSE text format
- Validating SSE message format
- Round-trip serialization/deserialization

## Test Cases

### 1. ParseEvent Tests

#### `ParseEvent_SimpleData`
- **Purpose**: Tests parsing basic SSE data without additional fields
- **Input**: `"data: Hello World\n\n"`
- **Expected**: Event with data="Hello World", empty event_type, no id/retry

#### `ParseEvent_WithEventType`
- **Purpose**: Tests parsing SSE with event type
- **Input**: `"event: message\ndata: Hello World\n\n"`
- **Expected**: Event with event_type="message", data="Hello World"

#### `ParseEvent_WithId`
- **Purpose**: Tests parsing SSE with event ID
- **Input**: `"id: 123\ndata: Hello World\n\n"`
- **Expected**: Event with id="123", data="Hello World"

#### `ParseEvent_WithRetry`
- **Purpose**: Tests parsing SSE with retry timeout
- **Input**: `"retry: 5000\ndata: Hello World\n\n"`
- **Expected**: Event with retry=5000, data="Hello World"

#### `ParseEvent_Comment`
- **Purpose**: Tests parsing SSE comment lines
- **Input**: `": This is a comment\n\n"`
- **Expected**: Event with data=": This is a comment"

#### `ParseEvent_MultiLineData`
- **Purpose**: Tests parsing multi-line data fields
- **Input**: `"data: Line 1\ndata: Line 2\n\n"`
- **Expected**: Event with data="Line 1\nLine 2"

#### `ParseEvent_CompleteEvent`
- **Purpose**: Tests parsing complete SSE event with all fields
- **Input**: `"event: message\nid: 123\nretry: 5000\ndata: Hello World\n\n"`
- **Expected**: Event with all fields populated

### 2. ParseEvents Tests

#### `ParseEvents_MultipleEvents`
- **Purpose**: Tests parsing multiple SSE events from a single text stream
- **Input**: Multiple events separated by double newlines
- **Expected**: Vector of 3 parsed events with correct field values

### 3. SerializeEvent Tests

#### `SerializeEvent_SimpleData`
- **Purpose**: Tests serializing basic event to SSE format
- **Input**: SseEvent with only data field
- **Expected**: `"data: Hello World\n\n"`

#### `SerializeEvent_WithEventType`
- **Purpose**: Tests serializing event with type
- **Input**: SseEvent with event_type and data
- **Expected**: `"event: message\ndata: Hello World\n\n"`

#### `SerializeEvent_WithId`
- **Purpose**: Tests serializing event with ID
- **Input**: SseEvent with id and data
- **Expected**: `"id: 123\ndata: Hello World\n\n"`

#### `SerializeEvent_WithRetry`
- **Purpose**: Tests serializing event with retry timeout
- **Input**: SseEvent with retry field
- **Expected**: `"retry: 5000\n\n"`

#### `SerializeEvent_Comment`
- **Purpose**: Tests serializing comment event
- **Input**: SseEvent with comment data
- **Expected**: `": This is a comment\n\n"`

#### `SerializeEvent_CompleteEvent`
- **Purpose**: Tests serializing complete event with all fields
- **Input**: SseEvent with all fields populated
- **Expected**: `"event: message\nid: 123\nretry: 5000\ndata: Hello World\n\n"`

### 4. SerializeEvents Tests

#### `SerializeEvents_MultipleEvents`
- **Purpose**: Tests serializing multiple events to SSE stream
- **Input**: Vector of 3 SseEvent objects
- **Expected**: Concatenated SSE text with all events

### 5. IsValidSseMessage Tests

#### `IsValidSseMessage_Valid`
- **Purpose**: Tests validation of valid SSE message
- **Input**: `"data: Hello World\n\n"`
- **Expected**: `true`

#### `IsValidSseMessage_Invalid`
- **Purpose**: Tests validation of invalid SSE message
- **Input**: `"invalid: field\n\n"`
- **Expected**: `false`

#### `IsValidSseMessage_Empty`
- **Purpose**: Tests validation of empty message
- **Input**: `""`
- **Expected**: `true`

#### `IsValidSseMessage_WithComments`
- **Purpose**: Tests validation of SSE with comments
- **Input**: `": Comment\ndata: Hello\n\n"`
- **Expected**: `true`

### 6. Round-Trip Tests

#### `RoundTrip_SimpleEvent`
- **Purpose**: Tests serialize → parse round-trip for simple event
- **Input**: SseEvent with basic data
- **Expected**: Parsed event matches original

#### `RoundTrip_ComplexEvent`
- **Purpose**: Tests serialize → parse round-trip for complex event
- **Input**: SseEvent with all fields
- **Expected**: All fields preserved through round-trip

## Running the Tests

### Prerequisites
- Bazel build system
- Google Test framework (automatically managed by Bazel)

### Commands

#### Build the test target
```bash
bazel build //trpc/util/http/sse/test:sse_parser_test
```

#### Run tests with minimal output
```bash
bazel test //trpc/util/http/sse/test:sse_parser_test
```

#### Run tests with detailed output
```bash
bazel test //trpc/util/http/sse/test:sse_parser_test --test_output=all
```

#### Run tests with verbose output
```bash
bazel test //trpc/util/http/sse/test:sse_parser_test --test_output=all --verbose_failures
```

#### Run specific test (if needed)
```bash
bazel test //trpc/util/http/sse/test:sse_parser_test --test_filter=SseParserTest.ParseEvent_SimpleData
```

## Expected Output

When running with `--test_output=all`, you should see:

```
Running main() from gmock_main.cc
[==========] Running 21 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 21 tests from SseParserTest
[ RUN      ] SseParserTest.ParseEvent_SimpleData
[       OK ] SseParserTest.ParseEvent_SimpleData (0 ms)
[ RUN      ] SseParserTest.ParseEvent_WithEventType
[       OK ] SseParserTest.ParseEvent_WithEventType (0 ms)
...
[ RUN      ] SseParserTest.RoundTrip_ComplexEvent
[       OK ] SseParserTest.RoundTrip_ComplexEvent (0 ms)
[----------] 21 tests from SseParserTest (0 ms total)

[----------] Global test environment tear-down
[==========] 21 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 21 tests.
```

## Test Coverage

The test suite provides comprehensive coverage for:

- ✅ **SSE Format Compliance**: All SSE field types (event, data, id, retry, comments)
- ✅ **Edge Cases**: Empty messages, multi-line data, missing fields
- ✅ **Error Handling**: Invalid message format detection
- ✅ **Round-trip Integrity**: Serialization/deserialization consistency
- ✅ **Multiple Events**: Parsing and serializing event streams

## Dependencies

- `//trpc/util/http/sse:http_sse` - The SSE library being tested
- `@com_google_googletest//:gtest` - Google Test framework
- `@com_google_googletest//:gtest_main` - Google Test main function

## Notes

- All tests use the `SseParserTest` test fixture
- Tests are designed to be independent and can run in any order
- SSE format follows the [W3C Server-Sent Events specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)
- Constructor ambiguity issues have been resolved by using explicit `std::string()` and `std::optional<>()` constructors
