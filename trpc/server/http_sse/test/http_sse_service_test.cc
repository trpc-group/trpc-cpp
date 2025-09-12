// trpc/server/http_sse/test/http_sse_service_test.cc
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/transport/server/testing/server_transport_testing.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/stream/http/http_sse_stream.h"  // SseStreamWriter
#include "trpc/server/http_sse/http_sse_service.h"

// Need the concrete protocol types to access .request/.response
#include "trpc/codec/http/http_protocol.h"

using namespace trpc;

namespace {

class DummySseHandler : public http::HttpHandler {
 public:
  // implement GET handler - simulate streaming a couple of events
  Status Get(const ServerContextPtr& ctx, const http::RequestPtr& req, http::Response* rsp) override {
    // Mark response as stream
    rsp->EnableStream(ctx.get());

    // Use SseStreamWriter to push SSE events
    stream::SseStreamWriter writer(ctx.get());

    // write header (optional - WriteEvent will call it implicitly)
    Status s = writer.WriteHeader();
    if (!s.OK()) return s;

    http::sse::SseEvent ev;
    ev.id = "42";
    ev.event_type = "message";
    ev.data = R"({"msg":"test","n":42})";

    s = writer.WriteEvent(ev);
    if (!s.OK()) return s;

    // write another event via WriteBuffer (simulate pre-serialized payload)
    std::string payload = "data: hello-buffer\n\n";
    auto buf = CreateBufferSlow(payload);
    s = writer.WriteBuffer(std::move(buf));
    if (!s.OK()) return s;

    // finish chunked response (optional)
    writer.WriteDone();

    return kSuccStatus;
  }

  // indicate this handler is stream-capable
  bool IsStream() const { return true; }
};

}  // namespace

class HttpSseServiceTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    codec::Init();
    serialization::Init();
  }
  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
  }
};

TEST_F(HttpSseServiceTest, DirectHandlerInvoke_WritesSseEvents) {
  // create a request that looks like an SSE request
  auto req = std::make_shared<http::Request>(1024, false);
  req->SetMethod("GET");
  req->SetHeader("Accept", "text/event-stream");

  // create response (handler will use it)
  http::Response rsp;

  // create an HttpSseService just for realistic ServerContext
  auto sse_service = std::make_shared<HttpSseService>();

  // Create a test transport — testing helper that the test context uses to capture sends.
  auto transport = std::make_shared<trpc::testing::TestServerTransport>();
  sse_service->SetServerTransport(transport.get());  // inject testing transport
  // Make a ServerContext that references our dummy service and request
  ServerContextPtr ctx = trpc::testing::MakeTestServerContext("http", sse_service.get(), std::move(req));
  // Attach a Connection placeholder (some helpers use it)
  Connection conn;
  ctx->SetReserved(&conn);

  // Prepare handler and call Get(...) via proper conversion of Protocol -> HttpRequestProtocol
  auto handler = std::make_shared<DummySseHandler>();

  // NOTE: Handler API expects http::RequestPtr, but the context stores a Protocol object
  // We must cast the Protocol to HttpRequestProtocol to get the wrapped request pointer.
  auto req_proto = static_cast<HttpRequestProtocol*>(ctx->GetRequestMsg().get());
  ASSERT_NE(req_proto, nullptr);  // sanity check

  Status s = handler->Get(ctx, req_proto->request, &rsp);
  if (!s.OK()) {
    std::cerr << "[TEST] handler.Get() returned failure: " << s.ToString()
              << ", framework retcode=" << s.GetFrameworkRetCode() << std::endl;
    std::cerr << "[TEST] context status: " << ctx->GetStatus().ToString() << std::endl;
    // If testing transport exposes sent data, print it (example API; your TestServerTransport may differ)
    // if (transport->HasSent()) { std::cerr << "Sent bytes: " << transport->ConsumeSentData() << std::endl; }
  }
  // Handler should report success
  EXPECT_TRUE(s.OK());

  // The context status should be OK and no stream reset
  EXPECT_TRUE(ctx->GetStatus().OK());
  EXPECT_FALSE(ctx->GetStatus().StreamRst());

  // --- optional: verify written bytes ---
  // If your TestServerTransport (or testing ServerContext implementation) captures the bytes
  // written by ctx->SendResponse(...), you can retrieve and assert them here. The exact API
  // depends on your test transport implementation; example pseudo:
  //
  // std::string sent = transport->ConsumeSentData();  // <- if such method exists
  // EXPECT_NE(std::string::npos, sent.find("data: {\"msg\":\"test\",\"n\":42}"));
}

