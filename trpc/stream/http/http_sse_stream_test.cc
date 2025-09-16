#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/http_service.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/transport/server/testing/server_transport_testing.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/stream/http/http_sse_stream.h"

namespace trpc::testing {

class SseStreamWriterTest : public ::testing::Test {
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

TEST_F(SseStreamWriterTest, WriteHeader) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());

  ASSERT_TRUE(writer.WriteHeader().OK());
  // Because the header is written directly through context_->SendResponse, we can only check the successful return here.
  // Whether the specific header is correct can be verified at the UT of the codec layer (SseResponseProtocol/HttpSseServerCodec)
}

TEST_F(SseStreamWriterTest, WriteEvent) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());

  using namespace trpc;
  using namespace trpc::http::sse;
  SseEvent ev{};
	
  ev.id = "1";
  ev.event_type = "message";
  ev.data = "hello world";

  ASSERT_TRUE(writer.WriteEvent(ev).OK());
  ASSERT_TRUE(writer.WriteDone().OK());
}

TEST_F(SseStreamWriterTest, WriteBuffer) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());

  // Directly construct a serialized SSE payload
  std::string payload = "id: 99\n"
                        "event: notice\n"
                        "data: pre-serialized\n\n";
  NoncontiguousBuffer buf = CreateBufferSlow(payload);

  ASSERT_TRUE(writer.WriteBuffer(std::move(buf)).OK());
  ASSERT_TRUE(writer.WriteDone().OK());
}

TEST_F(SseStreamWriterTest, Close) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());
  http::sse::SseEvent ev;
  ev.data = "bye";
  ASSERT_TRUE(writer.WriteEvent(ev).OK());

  // Close will call WriteDone + CloseConnection internally
  writer.Close();
}

}  // namespace trpc::testing

