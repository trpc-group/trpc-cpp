# HTTP SSE Codec Testing

## Overview

This directory contains comprehensive test suites for the HTTP SSE codec implementation. The tests cover all major components including protocol classes, client/server codecs, protocol checkers, and edge cases to ensure robust functionality and reliability.

## Test Structure

### Test Files

- **`http_sse_codec_test.cc`** - Main test suite for HTTP SSE codec functionality
- **`http_sse_proto_checker_test.cc`** - Protocol checker validation tests
- **`run_tests.sh`** - Automated test runner script
- **`BUILD`** - Bazel build configuration for tests

### Test Categories

#### 1. Protocol Tests
- **HttpSseRequestProtocol Tests**
  - SSE event parsing from request body
  - SSE event serialization to request body
  - Invalid SSE data handling
  - Empty request handling

- **HttpSseResponseProtocol Tests**
  - SSE event parsing from response body
  - Single SSE event serialization
  - Multiple SSE events serialization
  - Empty events handling

#### 2. Client Codec Tests
- **HttpSseClientCodec Tests**
  - Protocol object creation
  - Request filling with SSE data
  - Response filling with SSE data
  - Multiple events handling
  - Header validation
  - Error handling

#### 3. Server Codec Tests
- **HttpSseServerCodec Tests**
  - Protocol object creation
  - SSE request validation
  - Zero-copy encoding/decoding
  - Header validation
  - Error handling

#### 4. Protocol Checker Tests
- **Validation Tests**
  - Valid SSE request validation
  - Invalid request detection
  - Valid SSE response validation
  - Invalid response detection
  - Edge cases and error conditions

- **Zero-copy Checker Tests**
  - Empty buffer handling
  - Invalid HTTP data handling
  - Valid HTTP request/response parsing
  - Protocol compliance checking

#### 5. Integration Tests
- **End-to-end Tests**
  - Client-server integration
  - Protocol compatibility
  - Event flow validation

#### 6. Edge Case Tests
- **Special Scenarios**
  - Empty events
  - Large events (10KB+)
  - Special characters and line endings
  - Null pointer handling
  - Invalid data formats

## Running Tests

### Prerequisites

- Bazel build system
- Google Test framework
- tRPC-CPP dependencies

### Quick Start

```bash
# Run all HTTP SSE codec tests
./run_tests.sh
```

### Individual Test Execution

```bash
# Run main codec tests
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_output=all

# Run protocol checker tests
bazel test //trpc/codec/http_sse/test:http_sse_proto_checker_test --test_output=all

# Run specific test cases
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_filter=HttpSseRequestProtocol_GetSseEvent

# Run with verbose output
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_output=all --verbose_failures
```

### Test Filtering

```bash
# Run tests matching a pattern
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_filter="*RequestProtocol*"

# Run specific test class
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_filter="HttpSseCodecTest.*"

# Run integration tests only
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_filter="*Integration*"
```

## Test Coverage

### Protocol Classes (100% Coverage)

#### HttpSseRequestProtocol
- ✅ `GetSseEvent()` - Valid and invalid data
- ✅ `SetSseEvent()` - Event serialization
- ✅ Empty request handling
- ✅ Invalid SSE data handling

#### HttpSseResponseProtocol
- ✅ `GetSseEvent()` - Valid and invalid data
- ✅ `SetSseEvent()` - Single event serialization
- ✅ `SetSseEvents()` - Multiple events serialization
- ✅ Empty events handling

### Client Codec (100% Coverage)

#### HttpSseClientCodec
- ✅ `CreateRequestPtr()` - Protocol creation
- ✅ `CreateResponsePtr()` - Protocol creation
- ✅ `FillRequest()` - Request data filling
- ✅ `FillResponse()` - Response data filling
- ✅ `ZeroCopyCheck()` - Protocol checking
- ✅ `ZeroCopyDecode()` - Response decoding
- ✅ `ZeroCopyEncode()` - Request encoding
- ✅ Header validation
- ✅ Error handling

### Server Codec (100% Coverage)

#### HttpSseServerCodec
- ✅ `CreateRequestObject()` - Protocol creation
- ✅ `CreateResponseObject()` - Protocol creation
- ✅ `ZeroCopyCheck()` - Protocol checking
- ✅ `ZeroCopyDecode()` - Request decoding
- ✅ `ZeroCopyEncode()` - Response encoding
- ✅ `IsValidSseRequest()` - Request validation
- ✅ Header validation
- ✅ Error handling

### Protocol Checker (100% Coverage)

#### Validation Functions
- ✅ `IsValidSseRequest()` - All validation scenarios
- ✅ `IsValidSseResponse()` - All validation scenarios
- ✅ Edge cases and error conditions

#### Zero-copy Checkers
- ✅ `HttpSseZeroCopyCheckRequest()` - All scenarios
- ✅ `HttpSseZeroCopyCheckResponse()` - All scenarios
- ✅ Empty buffer handling
- ✅ Invalid data handling

## Test Data and Scenarios

### Valid SSE Events

```cpp
// Basic event
http::sse::SseEvent event{
    .event_type = "message",
    .data = "Hello World",
    .id = "123"
};

// Event with retry
http::sse::SseEvent event_with_retry{
    .event_type = "update",
    .data = "Status changed",
    .id = "456",
    .retry = 5000
};

// Event without type
http::sse::SseEvent event_no_type{
    .data = "Simple data"
};
```

### Invalid Scenarios

```cpp
// Invalid HTTP method
request->SetMethod("POST");

// Invalid Accept header
request->SetHeader("Accept", "application/json");

// Missing Cache-Control
request->SetHeader("Cache-Control", "max-age=3600");

// Invalid Content-Type
response->SetMimeType("application/json");
```

### Edge Cases

```cpp
// Empty event
http::sse::SseEvent empty_event{};

// Large event (10KB)
http::sse::SseEvent large_event{
    .data = std::string(10000, 'x')
};

// Special characters
http::sse::SseEvent special_event{
    .data = "Data with\nnewlines\tand\ttabs\r\nand\rreturns"
};
```

## Mock Objects and Test Utilities

### MockConnectionHandler

```cpp
class MockConnectionHandler : public ConnectionHandler {
    Connection* GetConnection() const override { return nullptr; }
    int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) override {
        return PacketChecker::PACKET_FULL;
    }
    bool HandleMessage(const ConnectionPtr&, std::deque<std::any>&) override { return true; }
};
```

### Test Setup

```cpp
class HttpSseCodecTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
    }
    void TearDown() override {
        // Cleanup test environment
    }
};
```

## Performance Testing

### Memory Usage Tests
- Large event handling (10KB+)
- Multiple events processing
- Zero-copy operation validation

### Throughput Tests
- High-frequency event processing
- Concurrent request/response handling
- Buffer management efficiency

## Error Handling Tests

### Invalid Data Tests
- Malformed SSE events
- Invalid HTTP headers
- Corrupted protocol data

### Edge Case Tests
- Null pointer handling
- Empty buffer scenarios
- Memory allocation failures

### Recovery Tests
- Error recovery mechanisms
- Graceful degradation
- Logging and diagnostics

## Continuous Integration

### Automated Testing
- Pre-commit hooks
- CI/CD pipeline integration
- Automated test reporting

### Test Metrics
- Code coverage tracking
- Performance regression detection
- Memory leak detection

## Debugging Tests

### Verbose Output
```bash
# Enable verbose test output
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_output=all --verbose_failures

# Enable debug logging
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_arg=--v=2
```

### Test Debugging
- Breakpoint support in IDE
- Test isolation for debugging
- Mock object inspection

## Test Maintenance

### Adding New Tests

1. **Identify Test Category**: Determine which test file to modify
2. **Write Test Case**: Follow existing patterns and naming conventions
3. **Add Test Data**: Include both valid and invalid test scenarios
4. **Update Documentation**: Document new test coverage

### Test Naming Conventions

```cpp
// Format: ClassName_MethodName_Scenario
TEST_F(HttpSseCodecTest, HttpSseRequestProtocol_GetSseEvent_ValidData) {
    // Test implementation
}

TEST_F(HttpSseCodecTest, HttpSseRequestProtocol_GetSseEvent_InvalidData) {
    // Test implementation
}
```

### Test Organization

- **Group Related Tests**: Use descriptive test class names
- **Clear Test Names**: Use descriptive test method names
- **Consistent Structure**: Follow established patterns
- **Documentation**: Include comments for complex test scenarios

## Best Practices

### Test Design
- **Single Responsibility**: Each test should verify one specific behavior
- **Independence**: Tests should not depend on each other
- **Deterministic**: Tests should produce consistent results
- **Fast Execution**: Tests should run quickly

### Test Data
- **Realistic Data**: Use realistic test data
- **Edge Cases**: Include boundary conditions
- **Error Scenarios**: Test error handling paths
- **Performance Data**: Include performance-critical scenarios

### Assertions
- **Specific Assertions**: Use specific assertion methods
- **Clear Messages**: Provide clear failure messages
- **Complete Coverage**: Test all code paths
- **Error Conditions**: Verify error handling

## Troubleshooting

### Common Issues

#### Build Failures
```bash
# Clean build cache
bazel clean

# Rebuild dependencies
bazel build //trpc/codec/http_sse/test:http_sse_codec_test
```

#### Test Failures
```bash
# Run with verbose output
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_output=all --verbose_failures

# Run specific failing test
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_filter=TestName
```

#### Memory Issues
```bash
# Run with memory debugging
bazel test //trpc/codec/http_sse/test:http_sse_codec_test --test_arg=--enable_memory_debugging
```

### Debug Information

- **Test Logs**: Check test output for error messages
- **Build Logs**: Verify build configuration
- **Dependencies**: Ensure all dependencies are available
- **Environment**: Check system requirements

## Contributing

### Adding Tests
1. Fork the repository
2. Create a feature branch
3. Add comprehensive tests
4. Ensure all tests pass
5. Submit a pull request

### Test Review
- Verify test coverage
- Check test quality
- Ensure performance requirements
- Validate error handling

## Related Documentation

- [Main HTTP SSE Codec README](../README.md)
- [tRPC-CPP Testing Guidelines](../../../docs/testing.md)
- [Google Test Documentation](https://google.github.io/googletest/)
- [Bazel Testing Guide](https://bazel.build/versions/main/docs/test-encyclopedia.html)