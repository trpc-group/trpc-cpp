# SSE Test Suite

This directory contains comprehensive tests for the Server-Sent Events (SSE) utilities in tRPC-Cpp.

## Overview

The test suite validates both the `SseEvent` struct and `SseParser` class functionality:

- **SseEvent Tests**: Struct creation, field access, and serialization via `ToString()`
- **SseParser Tests**: Parsing SSE text format into `SseEvent` objects and validation

## Test Structure

### 1. SseEvent Tests (`sse_event_test.cc`)

Tests for the `SseEvent` struct and its `ToString()` serialization method.

#### `ToStringBasicMessage`
- **Purpose**: Tests basic serialization with only data field
- **Input**: `SseEvent` with `data = "This is the first message."`
- **Expected**: `"data: This is the first message.\n\n"`

#### `ToStringWithEventType`
- **Purpose**: Tests serialization with event type
- **Input**: `SseEvent` with `event_type = "message"` and `data = "Hello World"`
- **Expected**: `"event: message\ndata: Hello World\n\n"`

#### `ToStringWithId`
- **Purpose**: Tests serialization with event ID
- **Input**: `SseEvent` with `id = "123"` and `data = "Hello World"`
- **Expected**: `"data: Hello World\nid: 123\n\n"`

#### `ToStringWithRetry`
- **Purpose**: Tests serialization with retry timeout
- **Input**: `SseEvent` with `retry = 5000` and `data = "Hello World"`
- **Expected**: `"data: Hello World\nretry: 5000\n\n"`

#### `ToStringCompleteEvent`
- **Purpose**: Tests serialization with all fields
- **Input**: `SseEvent` with all fields populated
- **Expected**: `"event: message\ndata: Hello World\nid: 123\nretry: 5000\n\n"`

#### `ToStringMultiLineData`
- **Purpose**: Tests serialization with multi-line data containing literal `\n` sequences
- **Input**: `SseEvent` with `data = "Line 1\\nLine 2\\nLine 3"`
- **Expected**: `"data: Line 1\ndata: Line 2\ndata: Line 3\n\n"`

#### `ToStringEmptyFields`
- **Purpose**: Tests serialization with empty optional fields
- **Input**: `SseEvent` with empty `event_type`, `id`, and `retry`
- **Expected**: `"data: Hello World\n\n"`

### 2. SseParser Tests (`sse_parser_test.cc`)

Tests for the `SseParser` class parsing and validation functionality.

#### ParseEvent Tests

##### `ParseEvent_SimpleData`
- **Purpose**: Tests parsing basic SSE data without additional fields
- **Input**: `"data: Hello World\n\n"`
- **Expected**: Event with `data="Hello World"`, empty `event_type`, no `id`/`retry`

##### `ParseEvent_WithEventType`
- **Purpose**: Tests parsing SSE with event type
- **Input**: `"event: message\ndata: Hello World\n\n"`
- **Expected**: Event with `event_type="message"`, `data="Hello World"`

##### `ParseEvent_WithId`
- **Purpose**: Tests parsing SSE with event ID
- **Input**: `"id: 123\ndata: Hello World\n\n"`
- **Expected**: Event with `id="123"`, `data="Hello World"`

##### `ParseEvent_WithRetry`
- **Purpose**: Tests parsing SSE with retry timeout
- **Input**: `"retry: 5000\ndata: Hello World\n\n"`
- **Expected**: Event with `retry=5000`, `data="Hello World"`

##### `ParseEvent_Comment`
- **Purpose**: Tests parsing SSE comment lines
- **Input**: `": This is a comment\n\n"`
- **Expected**: Event with `data=": This is a comment"`

##### `ParseEvent_MultiLineData`
- **Purpose**: Tests parsing multi-line data fields
- **Input**: `"data: Line 1\ndata: Line 2\n\n"`
- **Expected**: Event with `data="Line 1\nLine 2"`

##### `ParseEvent_CompleteEvent`
- **Purpose**: Tests parsing complete SSE event with all fields
- **Input**: `"event: message\nid: 123\nretry: 5000\ndata: Hello World\n\n"`
- **Expected**: Event with all fields populated

#### ParseEvents Tests

##### `ParseEvents_MultipleEvents`
- **Purpose**: Tests parsing multiple SSE events from a single text stream
- **Input**: Multiple events separated by double newlines
- **Expected**: Vector of 3 parsed events with correct field values

#### IsValidSseMessage Tests

##### `IsValidSseMessage_Valid`
- **Purpose**: Tests validation of valid SSE message
- **Input**: `"data: Hello World\n\n"`
- **Expected**: `true`

##### `IsValidSseMessage_Invalid`
- **Purpose**: Tests validation of invalid SSE message
- **Input**: `"invalid: field\n\n"`
- **Expected**: `false`

##### `IsValidSseMessage_Empty`
- **Purpose**: Tests validation of empty message
- **Input**: `""`
- **Expected**: `true`

##### `IsValidSseMessage_WithComments`
- **Purpose**: Tests validation of SSE with comments
- **Input**: `": Comment\ndata: Hello\n\n"`
- **Expected**: `true`

## Running the Tests

### Prerequisites
- Bazel build system
- Google Test framework (automatically managed by Bazel)

### Commands

#### Build all test targets
```bash
bazel build //trpc/util/http/sse/test/...
```

#### Run all SSE tests
```bash
bazel test //trpc/util/http/sse/test/...
```

#### Run specific test suites
```bash
# Run SseEvent tests only
bazel test //trpc/util/http/sse/test:sse_event_test

# Run SseParser tests only
bazel test //trpc/util/http/sse/test:sse_parser_test
```

#### Run tests with detailed output
```bash
bazel test //trpc/util/http/sse/test/... --test_output=all
```

#### Run tests with verbose output
```bash
bazel test //trpc/util/http/sse/test/... --test_output=all --verbose_failures
```

#### Run specific test (if needed)
```bash
# Run specific SseEvent test
bazel test //trpc/util/http/sse/test:sse_event_test --test_filter=SseEventTest.ToStringBasicMessage

# Run specific SseParser test
bazel test //trpc/util/http/sse/test:sse_parser_test --test_filter=SseParserTest.ParseEvent_SimpleData
```

## Expected Output

When running with `--test_output=all`, you should see:

```
Running main() from gmock_main.cc
[==========] Running 8 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 8 tests from SseEventTest
[ RUN      ] SseEventTest.ToStringBasicMessage
[       OK ] SseEventTest.ToStringBasicMessage (0 ms)
[ RUN      ] SseEventTest.ToStringWithEventType
[       OK ] SseEventTest.ToStringWithEventType (0 ms)
...
[----------] 8 tests from SseEventTest (0 ms total)

[----------] Global test environment tear-down
[==========] 8 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 8 tests.

Running main() from gmock_main.cc
[==========] Running 13 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 13 tests from SseParserTest
[ RUN      ] SseParserTest.ParseEvent_SimpleData
[       OK ] SseParserTest.ParseEvent_SimpleData (0 ms)
[ RUN      ] SseParserTest.ParseEvent_WithEventType
[       OK ] SseParserTest.ParseEvent_WithEventType (0 ms)
...
[----------] 13 tests from SseParserTest (0 ms total)

[----------] Global test environment tear-down
[==========] 13 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 13 tests.
```

## Test Coverage

The test suite provides comprehensive coverage for:

### SseEvent Tests
- ✅ **Struct Creation**: Direct member initialization
- ✅ **Field Access**: Direct member access (no getters/setters)
- ✅ **Serialization**: `ToString()` method with all field combinations
- ✅ **Multi-line Data**: Handling of literal `\n` sequences
- ✅ **Optional Fields**: Proper handling of empty optional fields

### SseParser Tests
- ✅ **SSE Format Compliance**: All SSE field types (event, data, id, retry, comments)
- ✅ **Edge Cases**: Empty messages, multi-line data, missing fields
- ✅ **Error Handling**: Invalid message format detection
- ✅ **Multiple Events**: Parsing event streams
- ✅ **Validation**: Message format validation

## Dependencies

- `//trpc/util/http/sse:http_sse` - The SseEvent struct
- `//trpc/util/http/sse:http_sse_parser` - The SseParser class
- `@com_google_googletest//:gtest` - Google Test framework
- `@com_google_googletest//:gtest_main` - Google Test main function

## Notes

- **SseEvent Tests**: Use `SseEventTest` test fixture
- **SseParser Tests**: Use `SseParserTest` test fixture
- **Test Independence**: All tests are designed to be independent and can run in any order
- **SSE Compliance**: Tests follow the [W3C Server-Sent Events specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)
- **Serialization**: Only `SseEvent::ToString()` is tested for serialization (no duplicate serialization methods)
- **Struct Usage**: Tests demonstrate proper struct member access patterns
