# HTTP SSE Codec Test Suite

This directory contains comprehensive tests for the HTTP Server-Sent Events (SSE) codec implementation in tRPC-Cpp.

## Overview

The test suite validates the HTTP SSE codec functionality, which includes:
- **HttpSseRequestProtocol**: SSE-specific request protocol handling
- **HttpSseResponseProtocol**: SSE-specific response protocol handling
- **HttpSseClientCodec**: Client-side SSE codec operations
- **HttpSseServerCodec**: Server-side SSE codec operations
- **Header Management**: SSE-specific HTTP header handling
- **Event Parsing**: SSE event parsing and serialization

## Test Cases

### 1. HttpSseRequestProtocol Tests

#### `HttpSseRequestProtocol_GetSseEvent`
- **Purpose**: Tests parsing SSE events from request bodies
- **Input**: Empty request, then request with valid SSE data
- **Expected**: Returns `std::nullopt` for empty request, parsed event for valid data

#### `HttpSseRequestProtocol_SetSseEvent`
- **Purpose**: Tests setting SSE events in request bodies
- **Input**: SseEvent with all fields (event_type, data, id, retry)
- **Expected**: Request contains serialized SSE data and proper content type

### 2. HttpSseResponseProtocol Tests

#### `HttpSseResponseProtocol_GetSseEvent`
- **Purpose**: Tests parsing SSE events from response bodies
- **Input**: Empty response, then response with valid SSE data
- **Expected**: Returns `std::nullopt` for empty response, parsed event for valid data

#### `HttpSseResponseProtocol_SetSseEvent`
- **Purpose**: Tests setting single SSE event in response body
- **Input**: SseEvent with event_type and data
- **Expected**: Response contains serialized SSE data and proper content type

#### `HttpSseResponseProtocol_SetSseEvents`
- **Purpose**: Tests setting multiple SSE events in response body
- **Input**: Vector of 3 SseEvent objects with different types
- **Expected**: Response contains concatenated SSE data with all events

### 3. HttpSseClientCodec Tests

#### `HttpSseClientCodec_CreateRequestPtr`
- **Purpose**: Tests creation of SSE request protocol objects
- **Input**: None
- **Expected**: Returns valid HttpSseRequestProtocol pointer

#### `HttpSseClientCodec_CreateResponsePtr`
- **Purpose**: Tests creation of SSE response protocol objects
- **Input**: None
- **Expected**: Returns valid HttpSseResponseProtocol pointer

#### `HttpSseClientCodec_FillRequest`
- **Purpose**: Tests filling SSE request with event data
- **Input**: SseEvent with event_type, data, and id
- **Expected**: Request contains SSE data and proper headers (Accept, Cache-Control, Connection)

#### `HttpSseClientCodec_FillResponse`
- **Purpose**: Tests extracting SSE events from response
- **Input**: Response protocol with SSE data
- **Expected**: Successfully extracts SseEvent with correct field values

### 4. HttpSseServerCodec Tests

#### `HttpSseServerCodec_CreateRequestObject`
- **Purpose**: Tests creation of SSE request protocol objects
- **Input**: None
- **Expected**: Returns valid HttpSseRequestProtocol pointer

#### `HttpSseServerCodec_CreateResponseObject`
- **Purpose**: Tests creation of SSE response protocol objects
- **Input**: None
- **Expected**: Returns valid HttpSseResponseProtocol pointer

#### `HttpSseServerCodec_IsValidSseRequest`
- **Purpose**: Tests SSE request validation logic
- **Input**: Valid SSE request (GET method, Accept: text/event-stream), invalid requests
- **Expected**: Returns true for valid requests, false for invalid ones

### 5. Codec Names Tests

#### `CodecNames`
- **Purpose**: Tests codec name consistency
- **Input**: HttpSseClientCodec and HttpSseServerCodec instances
- **Expected**: Both codecs return "http_sse" as their name

## Running the Tests

### Prerequisites
- Bazel build system
- Google Test framework (automatically managed by Bazel)
- tRPC-Cpp framework dependencies

### Commands

#### Build the test target
```bash
bazel build //trpc/codec/http_sse:http_sse_codec_test
```

#### Run tests with minimal output
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test
```

#### Run tests with detailed output
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_output=all
```

#### Run tests with verbose output
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_output=all --verbose_failures
```

#### Run specific test (if needed)
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_filter=HttpSseCodecTest.HttpSseRequestProtocol_GetSseEvent
```

#### Run tests with debug symbols
```bash
bazel test -c dbg //trpc/codec/http_sse:http_sse_codec_test --test_output=all
```

#### Run tests with optimizations
```bash
bazel test -c opt //trpc/codec/http_sse:http_sse_codec_test --test_output=all
```

## Expected Output

When running with `--test_output=all`, you should see:

```
Running main() from gmock_main.cc
[==========] Running 12 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 12 tests from HttpSseCodecTest
[ RUN      ] HttpSseCodecTest.HttpSseRequestProtocol_GetSseEvent
[       OK ] HttpSseCodecTest.HttpSseRequestProtocol_GetSseEvent (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseRequestProtocol_SetSseEvent
[       OK ] HttpSseCodecTest.HttpSseRequestProtocol_SetSseEvent (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseResponseProtocol_GetSseEvent
[       OK ] HttpSseCodecTest.HttpSseResponseProtocol_GetSseEvent (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseResponseProtocol_SetSseEvent
[       OK ] HttpSseCodecTest.HttpSseResponseProtocol_SetSseEvent (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseResponseProtocol_SetSseEvents
[       OK ] HttpSseCodecTest.HttpSseResponseProtocol_SetSseEvents (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseClientCodec_CreateRequestPtr
[       OK ] HttpSseCodecTest.HttpSseClientCodec_CreateRequestPtr (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseClientCodec_CreateResponsePtr
[       OK ] HttpSseCodecTest.HttpSseClientCodec_CreateResponsePtr (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseClientCodec_FillRequest
[       OK ] HttpSseCodecTest.HttpSseClientCodec_FillRequest (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseClientCodec_FillResponse
[       OK ] HttpSseCodecTest.HttpSseClientCodec_FillResponse (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseServerCodec_CreateRequestObject
[       OK ] HttpSseCodecTest.HttpSseServerCodec_CreateRequestObject (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseServerCodec_CreateResponseObject
[       OK ] HttpSseCodecTest.HttpSseServerCodec_CreateResponseObject (0 ms)
[ RUN      ] HttpSseCodecTest.HttpSseServerCodec_IsValidSseRequest
[       OK ] HttpSseCodecTest.HttpSseServerCodec_IsValidSseRequest (0 ms)
[ RUN      ] HttpSseCodecTest.CodecNames
[       OK ] HttpSseCodecTest.CodecNames (0 ms)
[----------] 12 tests from HttpSseCodecTest (0 ms total)

[----------] Global test environment tear-down
[==========] 12 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 12 tests.
```

**Note**: All tests should pass successfully after the recent compilation fixes.

## Recent Fixes and Improvements

### Compilation Issues Resolved
- **Constructor Simplification**: Simplified SseEvent to use a single constructor, eliminating ambiguity
- **Test Fixture Names**: Corrected test fixture names from `HttpSseClientCodecTest` to `HttpSseCodecTest`
- **Method Visibility**: Made `IsValidSseRequest` method public for testing access
- **Header Method Calls**: Fixed HTTP header setting methods to use correct API calls

### Code Quality Improvements
- **Type Safety**: Enhanced type safety with explicit casting
- **Test Coverage**: Comprehensive coverage of all codec functionality
- **Error Handling**: Proper error handling and validation in tests
- **Documentation**: Updated documentation to reflect current implementation

## Test Coverage

The test suite provides comprehensive coverage for:

- ✅ **Protocol Classes**: Request/response protocol functionality
- ✅ **Codec Operations**: Encoding/decoding operations
- ✅ **Header Management**: SSE-specific header handling
- ✅ **Event Parsing**: SSE event parsing and serialization
- ✅ **Request Validation**: SSE request validation logic
- ✅ **Object Creation**: Protocol object instantiation
- ✅ **Error Handling**: Graceful handling of invalid data

## Test Data Examples

### Valid SSE Event
```cpp
http::sse::SseEvent event("message", "Hello World", "123", 5000);
// event_type: "message"
// data: "Hello World"
// id: "123"
// retry: 5000
```

### Valid SSE Request Headers
```
Accept: text/event-stream
Cache-Control: no-cache
Connection: keep-alive
```

### Valid SSE Response Headers
```
Content-Type: text/event-stream
Cache-Control: no-cache
Connection: keep-alive
Access-Control-Allow-Origin: *
Access-Control-Allow-Headers: Cache-Control
```

## Dependencies

The test suite depends on:
- `//trpc/codec/http_sse:http_sse_codec` - The SSE codec being tested
- `//trpc/codec/http:http_protocol` - HTTP protocol definitions
- `//trpc/client:client_context` - Client context types
- `//trpc/server:server_context` - Server context types
- `//trpc/util/http/sse:http_sse` - SSE utilities
- `@com_google_googletest//:gtest` - Google Test framework
- `@com_google_googletest//:gtest_main` - Google Test main function

### Build Dependencies
- **C++17**: Required for `std::optional` and other modern C++ features
- **Bazel**: Build system for dependency management
- **Google Test**: Testing framework for C++

## Troubleshooting

### Common Issues

1. **Build Failures**: Ensure all dependencies are properly installed
2. **Test Failures**: Check that SSE utilities are working correctly
3. **Header Issues**: Verify HTTP header handling is consistent
4. **Constructor Usage**: SseEvent uses a single constructor with optional parameters
5. **Private Method Access**: Ensure test methods are accessible (IsValidSseRequest is now public)

### Debug Commands

```bash
# Build with debug information
bazel build -c dbg //trpc/codec/http_sse:http_sse_codec_test

# Run with verbose output
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_output=all --verbose_failures

# Run specific failing test
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_filter=HttpSseCodecTest.HttpSseRequestProtocol_GetSseEvent --test_output=all
```

## Notes

- All tests use the `HttpSseCodecTest` test fixture
- Tests are designed to be independent and can run in any order
- SSE format follows the [W3C Server-Sent Events specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)
- Tests validate both client-side and server-side SSE codec functionality
- **Constructor Design**: SseEvent uses a single constructor with optional parameters for simplicity
- **Method Visibility**: `IsValidSseRequest` method is public for testing purposes
- **Header Management**: Tests verify proper SSE-specific HTTP headers are set

## Related Documentation

- [HTTP SSE Codec README](../README.md) - Main codec documentation
- [SSE Utilities README](../../util/http/sse/README.md) - Core SSE utilities
- [tRPC-Cpp Framework](https://github.com/trpc-group/trpc-cpp) - Main framework documentation
