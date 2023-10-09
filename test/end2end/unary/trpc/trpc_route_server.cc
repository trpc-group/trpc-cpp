// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#include "test/end2end/unary/trpc/trpc_route_server.h"

#include <unordered_map>

#include "test/end2end/unary/trpc/common_funcs.h"
#include "test/end2end/unary/trpc/trpc_test.trpc.pb.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/server/forward/forward_rpc_method_handler.h"
#include "trpc/server/forward/forward_rpc_service_impl.h"

namespace trpc::testing {

using namespace ::trpc::testing::unarytrpc;

namespace {
// used to record the timeout error code of the last call on the server side
uint32_t server_last_ret_code = 0;
// used to record the timeout error code of the last call on the client side
uint32_t client_last_ret_code = 0;

// timeout error callback function of server side
void ServiceTimeoutFunc(const ::trpc::ServerContextPtr& context) {
  server_last_ret_code = context->GetStatus().GetFrameworkRetCode();
}

// timeout error callback function of client side
void ClientTimeoutFunc(const ::trpc::ClientContextPtr& context) {
  client_last_ret_code = context->GetStatus().GetFrameworkRetCode();
}
}  // namespace

class RouteTestServiceImpl : public trpc::testing::unarytrpc::RouteTest {
 public:
  RouteTestServiceImpl() {
    ::trpc::ServiceProxyOption option;
    option.proxy_callback.client_timeout_handle_function = ClientTimeoutFunc;
    proxy_ = ::trpc::GetTrpcClient()->GetProxy<TrpcTestServiceProxy>("trpc_test_client", &option);
  }

  ::trpc::Status TestTransInfo(::trpc::ServerContextPtr context, const RouteTransRequest* request,
                               RouteTransResponse* response) {
    if (request->test_case_type() == RouteTransRequest::USE_SERVER_CONTEXT) {
      auto client_ctx = ::trpc::MakeClientContext(context, proxy_);
      return CallServerTestTransinfo(request, response, context, client_ctx);
    } else if (request->test_case_type() == RouteTransRequest::NOT_USE_SERVER_CONTEXT) {
      auto client_ctx = ::trpc::MakeClientContext(proxy_);
      return CallServerTestTransinfo(request, response, context, client_ctx);
    }
    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestTimeout(::trpc::ServerContextPtr context, const TimeoutRequest* request,
                             TimeoutResponse* response) {
    ::trpc::testing::SleepFor(request->route_deal_time());

    auto client_ctx = ::trpc::MakeClientContext(context, proxy_);
    auto status = CallServerTestTimeout(request, response, context, client_ctx);
    return status;
  }

  ::trpc::Status QueryLastTimeoutInfo(::trpc::ServerContextPtr context, const QueryTimeoutRequest* request,
                                      QueryTimeoutResponse* response) {
    response->set_server_last_ret_code(server_last_ret_code);
    response->set_client_last_ret_code(client_last_ret_code);
    server_last_ret_code = 0;
    client_last_ret_code = 0;
    return ::trpc::kSuccStatus;
  }

 private:
  ::trpc::Status CallServerTestTransinfo(const RouteTransRequest* request, RouteTransResponse* response,
                                         ::trpc::ServerContextPtr& server_ctx, ::trpc::ClientContextPtr& client_ctx) {
    if (::trpc::runtime::IsInFiberRuntime()) {
      return SyncCallServerTestTransinfo(request, response, client_ctx);
    } else {
      return AsyncCallServerTestTransinfo(request, server_ctx, client_ctx);
    }
  }

  ::trpc::Status SyncCallServerTestTransinfo(const RouteTransRequest* request, RouteTransResponse* response,
                                             ::trpc::ClientContextPtr& client_ctx) {
    TransRequest trans_request;
    trans_request.set_test_case_type(TransRequest::DIFFERENT_KEY_USE_SET_INTERFACE);
    trans_request.set_key_prefix(request->key_prefix());
    TransResponse trans_reply;
    auto status = proxy_->TestTransInfo(client_ctx, trans_request, &trans_reply);
    if (status.OK()) {
      for (const auto& kv : trans_reply.recv_kv()) {
        auto new_kv = response->add_recv_kv();
        new_kv->set_key(kv.key());
        new_kv->set_value(kv.value());
      }
    }
    return status;
  }

  ::trpc::Status AsyncCallServerTestTransinfo(const RouteTransRequest* request, ::trpc::ServerContextPtr& server_ctx,
                                              ::trpc::ClientContextPtr& client_ctx) {
    server_ctx->SetResponse(false);
    TransRequest trans_request;
    trans_request.set_test_case_type(TransRequest::DIFFERENT_KEY_USE_SET_INTERFACE);
    trans_request.set_key_prefix(request->key_prefix());

    proxy_->AsyncTestTransInfo(client_ctx, trans_request).Then([server_ctx](trpc::Future<TransResponse>&& fut) mutable {
      trpc::Status status;
      RouteTransResponse response;

      if (fut.IsReady()) {
        TransResponse trans_reply = fut.GetValue0();
        for (const auto& kv : trans_reply.recv_kv()) {
          auto new_kv = response.add_recv_kv();
          new_kv->set_key(kv.key());
          new_kv->set_value(kv.value());
        }
      } else {
        status.SetErrorMessage(fut.GetException().what());
        status.SetFrameworkRetCode(fut.GetException().GetExceptionCode());
      }

      server_ctx->SendUnaryResponse(status, response);
      return trpc::MakeReadyFuture<>();
    });
    return trpc::Status();
  }

  ::trpc::Status CallServerTestTimeout(const TimeoutRequest* request, TimeoutResponse* response,
                                       ::trpc::ServerContextPtr& server_ctx, ::trpc::ClientContextPtr& client_ctx) {
    if (::trpc::runtime::IsInFiberRuntime()) {
      return proxy_->TestTimeout(client_ctx, *request, response);
    } else {
      server_ctx->SetResponse(false);
      proxy_->AsyncTestTimeout(client_ctx, *request).Then([server_ctx](trpc::Future<TimeoutResponse>&& fut) mutable {
        trpc::Status status;
        TimeoutResponse response;

        if (fut.IsReady()) {
          response = fut.GetValue0();
        } else {
          status.SetErrorMessage(fut.GetException().what());
          status.SetFrameworkRetCode(fut.GetException().GetExceptionCode());
        }

        server_ctx->SendUnaryResponse(status, response);
        return trpc::MakeReadyFuture<>();
      });
      return trpc::Status();
    }
  }

 private:
  std::shared_ptr<TrpcTestServiceProxy> proxy_;
};

class RouteTransparentService : public ::trpc::ForwardRpcServiceImpl {
 public:
  RouteTransparentService() {
    proxy_ = ::trpc::GetTrpcClient()->GetProxy<TrpcTestServiceProxy>("trpc_test_client");

    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        ::trpc::kTransparentRpcName, ::trpc::MethodType::UNARY,
        new ::trpc::ForwardRpcMethodHandler(std::bind(&RouteTransparentService::Forward, this, std::placeholders::_1,
                                                      std::placeholders::_2, std::placeholders::_3))));
  }

  ::trpc::Status Forward(::trpc::ServerContextPtr context, ::trpc::NoncontiguousBuffer&& request,
                         ::trpc::NoncontiguousBuffer& response) {
    const auto& trans_info = context->GetPbReqTransInfo();
    if (trans_info.find("MakeTransparentClientContext") != trans_info.end()) {
      ::trpc::ClientContextPtr client_context = ::trpc::MakeTransparentClientContext(context, proxy_);
      return InnerForward(request, response, context, client_context);
    } else if (trans_info.find("MakeClientContext") != trans_info.end()) {
      ::trpc::ClientContextPtr client_context = ::trpc::MakeClientContext(context, proxy_);
      return InnerForward(request, response, context, client_context);
    }

    return ::trpc::kUnknownErrorStatus;
  }

 private:
  ::trpc::Status InnerForward(::trpc::NoncontiguousBuffer& request, ::trpc::NoncontiguousBuffer& response,
                              ::trpc::ServerContextPtr& server_ctx, ::trpc::ClientContextPtr& client_ctx) {
    if (::trpc::runtime::IsInFiberRuntime()) {
      return proxy_->UnaryInvoke(client_ctx, request, &response);
    } else {
      server_ctx->SetResponse(false);
      proxy_->AsyncUnaryInvoke<::trpc::NoncontiguousBuffer, ::trpc::NoncontiguousBuffer>(client_ctx, request)
          .Then([server_ctx, this](trpc::Future<trpc::NoncontiguousBuffer>&& fut) mutable {
            trpc::Status status;
            trpc::NoncontiguousBuffer reply;

            if (fut.IsFailed()) {
              status.SetErrorMessage(fut.GetException().what());
              status.SetFrameworkRetCode(fut.GetException().GetExceptionCode());
            } else {
              reply = fut.GetValue0();
            }

            server_ctx->SendTransparentResponse(status, std::move(reply));
            return trpc::MakeReadyFuture<>();
          });
      return trpc::Status();
    }
  }

 private:
  std::shared_ptr<TrpcTestServiceProxy> proxy_;
};

int RouteTestServer::Initialize() {
  auto route_service = std::make_shared<RouteTestServiceImpl>();
  route_service->SetServiceTimeoutHandleFunction(ServiceTimeoutFunc);
  RegisterService("route_service", route_service);

  auto no_fulllink_route_service = std::make_shared<RouteTestServiceImpl>();
  no_fulllink_route_service->SetServiceTimeoutHandleFunction(ServiceTimeoutFunc);
  RegisterService("no_fulllinlk_route_service", no_fulllink_route_service);

  RegisterService("route_transparent_service", std::make_shared<RouteTransparentService>());

  server_->Start();

  test_signal_->SignalClientToContinue();

  return 0;
}

extern "C" void __gcov_flush();

void RouteTestServer::Destroy() { __gcov_flush(); }

}  // namespace trpc::testing
