# README — HTTP SSE tests 

**Location:** `trpc/server/http_sse/test`

This document describes two unit tests that exercise the server-side SSE stream writer and a simple SSE-capable service handler. It explains what the tests do, how to build & run them with Bazel, expected outputs, troubleshooting tips and suggestions for extension.

---

## Files in this folder

* **`http_sse_stream_parser_test.cc`**
  Tests `trpc::stream::SseStreamWriter` output at the *wire* level and validates that the client-side parser (`trpc::http::sse::SseParser`) can decode the chunked HTTP body and produce correct SSE events.

* **`http_sse_service_test.cc`**
  Tests `HttpSseService` handler behavior by directly invoking a stream-capable handler (`DummySseHandler`) that uses `SseStreamWriter` to emit SSE payloads. Verifies the handler returns success and the server context remains healthy.

---

# What each test does (high level)

## `http_sse_stream_parser_test.cc`

1. Creates a `MockServerContext` that captures bytes passed to `SendResponse(NoncontiguousBuffer)`.
2. Creates `trpc::stream::SseStreamWriter` bound to the mock context.
3. Writes header, writes SSE event(s) (via `WriteEvent` or `WriteBuffer`), and calls `WriteDone`.
4. From captured bytes: finds header/body separator, decodes HTTP **chunked** body into concatenated payload(s).
5. Uses `trpc::http::sse::SseParser::ParseEvents` to parse SSE text into `SseEvent` objects.
6. Asserts `id`, `event_type` and `data` match expected values.

**Purpose:** verifies `SseStreamWriter` produces a valid HTTP header + chunked body where the chunked payload is properly formatted SSE text and is parsable by the SSE parser.

## `http_sse_service_test.cc`

1. Initializes codec and serialization subsystems required by the framework.
2. Builds a mock `ServerContext` (via test helpers) and constructs a `DummySseHandler` that:

   * marks the response as streaming (`rsp->EnableStream(ctx)`),
   * writes header and SSE events using `SseStreamWriter`,
   * finishes with `WriteDone`.
3. Calls `handler->Get(...)` directly and asserts `Status::OK()` and that `ServerContext` has no stream-reset condition.

**Purpose:** verifies that inside a ServerContext and handler, `SseStreamWriter` can be used and completes without framework-level failures,test ` http_sse_service.cc` .

---

# Build & run (Bazel)

From the repository root, build and run the tests:

### Run the stream parser test

```bash
bazel build //trpc/server/http_sse/test:http_sse_stream_parser_test 
bazel test //trpc/server/http_sse/test:http_sse_stream_parser_test --test_output=all
```

### Run the service test

```bash
bazel build //trpc/server/http_sse/test:http_sse_service_test 
bazel test //trpc/server/http_sse/test:http_sse_service_test --test_output=all
```

---

# Expected output
When tests succeed, Bazel will show output similar to:

```
==================== Test output for //trpc/server/http_sse/test:http_sse_stream_parser_test:
Running main() from gmock_main.cc
[==========] Running 2 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 2 tests from SseStreamWriter_SseParser_Test
[ RUN      ] SseStreamWriter_SseParser_Test.WriteEventAndClientParse
[       OK ] SseStreamWriter_SseParser_Test.WriteEventAndClientParse (2 ms)
[ RUN      ] SseStreamWriter_SseParser_Test.WriteBufferAndClientParse
[       OK ] SseStreamWriter_SseParser_Test.WriteBufferAndClientParse (0 ms)
[----------] 2 tests from SseStreamWriter_SseParser_Test (2 ms total)

[----------] Global test environment tear-down
[==========] 2 tests from 1 test suite ran. (2 ms total)
[  PASSED  ] 2 tests.
================================================================================

==================== Test output for //trpc/server/http_sse/test:http_sse_service_test:
Running main() from gmock_main.cc
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from HttpSseServiceTest
[ RUN      ] HttpSseServiceTest.DirectHandlerInvoke_WritesSseEvents
[       OK ] HttpSseServiceTest.DirectHandlerInvoke_WritesSseEvents (2 ms)
[----------] 1 test from HttpSseServiceTest (2 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (2 ms total)
[  PASSED  ] 1 test.
================================================================================

