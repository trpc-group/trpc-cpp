// http_sse_stream_parser_test.cc
#include "gtest/gtest.h"

#include <memory>
#include <string>
#include <any>
#include <iostream>

#include "trpc/stream/http/http_sse_stream.h"        // SseStreamWriter
#include "trpc/util/http/sse/sse_parser.h"          // SseParser
#include "trpc/util/http/sse/sse_event.h"           // SseEvent
#include "trpc/util/buffer/noncontiguous_buffer.h"  // NoncontiguousBuffer
#include "trpc/filter/server_filter_controller.h"
#include "trpc/server/server_context.h"
#include "trpc/common/status.h"

using namespace trpc;

//
// A small mock ServerContext that captures SendResponse() output into a string
// so the test can examine the raw bytes the SseStreamWriter wrote.
//
class MockServerContext : public ServerContext {
 public:
  MockServerContext() : user_data_(0), closed_(false) {
    // provide a filter controller because some code asserts on it
    server_filter_controller_ = std::make_unique<ServerFilterController>();
    SetFilterController(server_filter_controller_.get());
  }

  // Capture the outgoing bytes: append all buffer blocks to captured_
  Status SendResponse(NoncontiguousBuffer&& buf) {
    // Append bytes from each block
    for (const auto& block : buf) {
      captured_.append(block.data(), block.size());
    }
    return Status();  // OK
  }

  // Minimal CloseConnection for tests
  void CloseConnection() {
    closed_ = true;
  }

  // Make HasFilterController return true for safety (some code logs/warns otherwise)
  bool HasFilterController() const { return true; }

  // User-data helpers (not used heavily in this test)
  void SetUserData(uint64_t id) { user_data_ = id; }
  std::any GetUserData() const { return user_data_; }

  const std::string& Captured() const { return captured_; }
  void ClearCaptured() { captured_.clear(); }
  bool Closed() const { return closed_; }

 private:
  std::unique_ptr<ServerFilterController> server_filter_controller_;
  std::string captured_;
  uint64_t user_data_;
  bool closed_;
};

/// Helper: find header/body separator (CRLFCRLF or LF LF)
static size_t FindHeaderEnd(const std::string& wire) {
  size_t pos = wire.find("\r\n\r\n");
  if (pos != std::string::npos) return pos + 4;
  pos = wire.find("\n\n");
  if (pos != std::string::npos) return pos + 2;
  return std::string::npos;
}

/// Helper: read a single line starting at idx; supports CRLF or LF; returns line (without CRLF) and advances idx
static std::string ReadLine(const std::string& s, size_t& idx) {
  if (idx >= s.size()) return "";
  size_t start = idx;
  size_t pos_crlf = s.find("\r\n", idx);
  size_t pos_lf = s.find('\n', idx);
  size_t end;
  if (pos_crlf != std::string::npos && (pos_crlf < pos_lf || pos_lf == std::string::npos)) {
    end = pos_crlf;
    idx = pos_crlf + 2;
  } else if (pos_lf != std::string::npos) {
    end = pos_lf;
    idx = pos_lf + 1;
  } else {
    // last line without newline
    end = s.size();
    idx = s.size();
  }
  return s.substr(start, end - start);
}

/// Helper: decode HTTP chunked body from wire (string starting at pos - expected to point to first chunk header)
/// Returns concatenated payload (all chunk data joined), and sets pos after the terminating 0 chunk (or at end).
static std::string DecodeChunkedPayload(const std::string& wire, size_t pos, bool* ok_out = nullptr) {
  std::string payload;
  bool ok = true;
  while (pos < wire.size()) {
    // read chunk-size line
    std::string line = ReadLine(wire, pos);
    if (line.empty()) {
      // could be stray empty line; skip
      continue;
    }
    // chunk-size may have extensions -> take until first ';'
    size_t semi = line.find(';');
    std::string hex = (semi == std::string::npos) ? line : line.substr(0, semi);
    // trim spaces
    auto trim = [](std::string &x){
      size_t a = 0, b = x.size();
      while (a < b && isspace((unsigned char)x[a])) ++a;
      while (b > a && isspace((unsigned char)x[b-1])) --b;
      x = x.substr(a, b-a);
    };
    trim(hex);
    if (hex.empty()) { ok = false; break; }
    // parse hex
    size_t chunk_size = 0;
    try {
      chunk_size = std::stoul(hex, nullptr, 16);
    } catch (...) {
      ok = false;
      break;
    }
    if (chunk_size == 0) {
      // end of chunks; there may be trailing header lines until blank line; consume them
      // consume possible trailing CRLF or trailer headers (read until blank line)
      size_t saved = pos;
      // try to read until blank line or end
      while (pos < wire.size()) {
        std::string l = ReadLine(wire, pos);
        if (l.empty()) break;
      }
      break;
    }
    // Now read exactly chunk_size bytes from pos
    if (pos + chunk_size > wire.size()) { ok = false; break; }
    payload.append(wire.data() + pos, chunk_size);
    pos += chunk_size;
    // after chunk data there should be CRLF; consume one CRLF or LF
    if (pos + 1 <= wire.size() && wire[pos] == '\r' && pos + 1 < wire.size() && wire[pos+1] == '\n') {
      pos += 2;
    } else if (pos < wire.size() && wire[pos] == '\n') {
      pos += 1;
    } else {
      // missing CRLF but continue best-effort
    }
  }
  if (ok_out) *ok_out = ok;
  return payload;
}


/// Test: SseStreamWriter writes header + one event chunk + final chunk; the client can decode chunked body and parse SSE.
TEST(SseStreamWriter_SseParser_Test, WriteEventAndClientParse) {
  MockServerContext ctx;
  // SseStreamWriter expects ServerContext*, our SseStreamWriter is in namespace trpc::stream
  trpc::stream::SseStreamWriter writer(&ctx);

  // Build event
  trpc::http::sse::SseEvent ev;
  ev.id = "42";
  ev.event_type = "notice";
  ev.data = "hello-client";

  // Write header then event
  ASSERT_TRUE(writer.WriteHeader().OK());
  ASSERT_TRUE(writer.WriteEvent(ev).OK());
  // also finish
  ASSERT_TRUE(writer.WriteDone().OK());

  // We have captured raw bytes (header + chunked body) in ctx.Captured()
  std::string wire = ctx.Captured();
  ASSERT_FALSE(wire.empty());

  // Find end of headers
  size_t body_start = FindHeaderEnd(wire);
  ASSERT_NE(body_start, std::string::npos) << "No header/body separator found in wire: [" << wire << "]";

  bool decode_ok = false;
  std::string sse_text = DecodeChunkedPayload(wire, body_start, &decode_ok);
  ASSERT_TRUE(decode_ok) << "Chunked decode failed";

  // Now parse SSE events using SseParser
  auto events = trpc::http::sse::SseParser::ParseEvents(sse_text);
  ASSERT_EQ(events.size(), 1u);
  auto &e0 = events[0];
  EXPECT_EQ(e0.id, "42");
  EXPECT_EQ(e0.event_type, "notice");
  EXPECT_EQ(e0.data, "hello-client");
}

/// Test: WriteBuffer (pre-serialized SSE text) and parse on client side
TEST(SseStreamWriter_SseParser_Test, WriteBufferAndClientParse) {
  MockServerContext ctx;
  trpc::stream::SseStreamWriter writer(&ctx);

  // Pre-serialized SSE payload (two events concatenated)
  std::string payload =
      "id: 99\n"
      "event: notice\n"
      "data: pre-serialized\n\n"
      "id: 100\n"
      "event: info\n"
      "data: line1\n"
      "data: line2\n\n";

  // Create NoncontiguousBuffer from string
  trpc::NoncontiguousBuffer buf;
  // There's an easy helper CreateBufferSlow in your codebase normally; but to avoid relying on it, we build one block:
  buf.Append(trpc::CreateBufferSlow(payload));  // CreateBufferSlow returns a NoncontiguousBuffer containing the data

  ASSERT_TRUE(writer.WriteHeader().OK());
  ASSERT_TRUE(writer.WriteBuffer(std::move(buf)).OK());
  ASSERT_TRUE(writer.WriteDone().OK());

  std::string wire = ctx.Captured();
  ASSERT_FALSE(wire.empty());

  size_t body_start = FindHeaderEnd(wire);
  ASSERT_NE(body_start, std::string::npos);

  bool decode_ok = false;
  std::string sse_text = DecodeChunkedPayload(wire, body_start, &decode_ok);
  ASSERT_TRUE(decode_ok);

  auto events = trpc::http::sse::SseParser::ParseEvents(sse_text);
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].id, "99");
  EXPECT_EQ(events[0].event_type, "notice");
  EXPECT_EQ(events[0].data, "pre-serialized");
  EXPECT_EQ(events[1].id, "100");
  EXPECT_EQ(events[1].event_type, "info");
  EXPECT_EQ(events[1].data, "line1\nline2");
}

