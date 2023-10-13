// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#include "test/end2end/unary/trpc/trpc_server.h"

#include <sstream>
#include <unordered_map>

#include "test/end2end/common/util.h"
#include "test/end2end/unary/trpc/common_funcs.h"
#include "test/end2end/unary/trpc/trpc_server_transport_test_inc.h"
#include "test/end2end/unary/trpc/trpc_test.trpc.fb.h"
#include "test/end2end/unary/trpc/trpc_test.trpc.pb.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/future/async_timer.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/init_runtime.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"
#include "trpc/server/rpc_method_handler.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"
#include "trpc/util/function.h"
#include "trpc/util/string_helper.h"

namespace trpc::testing {

using namespace ::trpc::testing::unarytrpc;

namespace {
// used to record the timeout error code of the last call on the server side
uint32_t last_ret_code = 0;

// timeout error callback function of server side
void ServiceTimeoutFunc(const ::trpc::ServerContextPtr& context) {
  last_ret_code = context->GetStatus().GetFrameworkRetCode();
}
}  // namespace

class TrpcTestServiceImpl : public trpc::testing::unarytrpc::TrpcTest {
 public:
  TrpcTestServiceImpl() {
    // initialize compressor_test_impl_map_
    compressor_test_impl_map_[CompressorRequest::DEFAULT] =
        [](::trpc::ServerContextPtr context, const CompressorRequest* request, CompressorResponse* response) {
          return ::trpc::kSuccStatus;
        };
    compressor_test_impl_map_[CompressorRequest::SPECIFY_LEVEL] =
        [](::trpc::ServerContextPtr context, const CompressorRequest* request, CompressorResponse* response) {
          // modify compression level to the expected value
          context->SetRspCompressLevel(static_cast<uint8_t>(request->expected_compress_level()));
          return ::trpc::kSuccStatus;
        };
    compressor_test_impl_map_[CompressorRequest::SPECIFY_ALGORITHM] =
        [](::trpc::ServerContextPtr context, const CompressorRequest* request, CompressorResponse* response) {
          // modify compression type and level to the expected values
          context->SetRspCompressType(static_cast<uint8_t>(request->expected_compress_type()));
          context->SetRspCompressLevel(static_cast<uint8_t>(request->expected_compress_level()));
          return ::trpc::kSuccStatus;
        };
    compressor_test_impl_map_[CompressorRequest::NOT_COMPRESS] =
        [](::trpc::ServerContextPtr context, const CompressorRequest* request, CompressorResponse* response) {
          // do not use compressor to respond
          context->SetRspCompressType(::trpc::compressor::kNone);
          return ::trpc::kSuccStatus;
        };
    compressor_test_impl_map_[CompressorRequest::INVALID_TYPE] =
        [](::trpc::ServerContextPtr context, const CompressorRequest* request, CompressorResponse* response) {
          // use invalid compression type
          context->SetRspCompressType(std::numeric_limits<uint8_t>::max());
          context->SetRspCompressLevel(::trpc::compressor::kDefault);
          return ::trpc::kSuccStatus;
        };
    compressor_test_impl_map_[CompressorRequest::INVALID_LEVEL] =
        [](::trpc::ServerContextPtr context, const CompressorRequest* request, CompressorResponse* response) {
          context->SetRspCompressType(::trpc::compressor::kGzip);
          // use invalid compression level
          context->SetRspCompressLevel(std::numeric_limits<uint8_t>::max());
          return ::trpc::kSuccStatus;
        };

    // initialize pb_serialization_test_impl_map_
    pb_serialization_test_impl_map_[SerializationRequest::PB_PB] =
        [](::trpc::ServerContextPtr context, const SerializationRequest* request, SerializationResponse* response) {
          return ::trpc::kSuccStatus;
        };
    pb_serialization_test_impl_map_[SerializationRequest::PB_NONCONTIGUOUSBUFFER] =
        [](::trpc::ServerContextPtr context, const SerializationRequest* request, SerializationResponse* response) {
          context->SetResponse(false);
          ::trpc::serialization::Serialization* pb_serialization =
              ::trpc::serialization::SerializationFactory::GetInstance()->Get(::trpc::EncodeType::PB);
          ::trpc::NoncontiguousBuffer rsp_buffer;
          bool encode_ret = pb_serialization->Serialize(trpc::serialization::kPbMessage,
                                                        reinterpret_cast<void*>(response), &rsp_buffer);
          if (encode_ret) {
            context->SendUnaryResponse(::trpc::kSuccStatus, rsp_buffer);
          } else {
            context->SendUnaryResponse(
                ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_ENCODE_ERR, 0, "server encode error"), rsp_buffer);
          }
          return ::trpc::kSuccStatus;
        };
    pb_serialization_test_impl_map_[SerializationRequest::INVALID_DATA] =
        [](::trpc::ServerContextPtr context, const SerializationRequest* request, SerializationResponse* response) {
          context->SetResponse(false);
          ::trpc::NoncontiguousBuffer rsp_buffer = ::trpc::CreateBufferSlow("invalid data");
          context->SendUnaryResponse(::trpc::kSuccStatus, rsp_buffer);
          return ::trpc::kSuccStatus;
        };
    pb_serialization_test_impl_map_[SerializationRequest::INVALID_TYPE] =
        [](::trpc::ServerContextPtr context, const SerializationRequest* request, SerializationResponse* response) {
          context->SetResponse(false);
          std::string rsp_str = "invalid type";
          context->SendUnaryResponse(::trpc::kSuccStatus, rsp_str);
          return ::trpc::kSuccStatus;
        };
    pb_serialization_test_impl_map_[SerializationRequest::NOOP_STRING] =
        [](::trpc::ServerContextPtr context, const SerializationRequest* request, SerializationResponse* response) {
          context->SetResponse(false);
          std::string rsp_str = request->msg();
          context->SetRspEncodeType(::trpc::serialization::kNoopType);
          context->SendUnaryResponse(::trpc::kSuccStatus, rsp_str);
          return ::trpc::kSuccStatus;
        };
    pb_serialization_test_impl_map_[SerializationRequest::NOT_EXIST_TYPE] =
        [](::trpc::ServerContextPtr context, const SerializationRequest* request, SerializationResponse* response) {
          context->SetResponse(false);
          context->SetRspEncodeType(std::numeric_limits<uint8_t>::max());
          context->SendUnaryResponse(::trpc::kSuccStatus, *response);
          return ::trpc::kSuccStatus;
        };
  }

  ::trpc::Status TestCommon(::trpc::ServerContextPtr context, const CommonRequest* request, CommonReply* response) {
    response->set_msg(request->msg());
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestCompressor(::trpc::ServerContextPtr context, const CompressorRequest* request,
                                CompressorResponse* response) {
    if (compressor_test_impl_map_.find(request->test_case_type()) != compressor_test_impl_map_.end()) {
      response->set_msg(request->msg());
      return compressor_test_impl_map_[request->test_case_type()](context, request, response);
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestPbSerialization(::trpc::ServerContextPtr context, const SerializationRequest* request,
                                     SerializationResponse* response) {
    if (pb_serialization_test_impl_map_.find(request->test_case_type()) != pb_serialization_test_impl_map_.end()) {
      response->set_msg(request->msg());
      return pb_serialization_test_impl_map_[request->test_case_type()](context, request, response);
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestTransInfo(::trpc::ServerContextPtr context, const TransRequest* request, TransResponse* response) {
    // extracting information with a key prefix specified by the client
    for (const auto& [key, value] : context->GetPbReqTransInfo()) {
      if (::trpc::StartsWith(key, request->key_prefix())) {
        // // inform the client about which transinfo the server has received
        auto new_kv = response->add_recv_kv();
        new_kv->set_key(key);
        new_kv->set_value(value);
      }
    }
    response->set_key_prefix("rsp-key-");

    if (request->test_case_type() == TransRequest::DIFFERENT_KEY_USE_SET_INTERFACE) {
      std::unordered_map<std::string, std::string> rsp_map;
      rsp_map["rsp-key-1"] = "rsp-value-1";
      rsp_map["rsp-key-2"] = "rsp-value-2";
      context->SetRspTransInfo(rsp_map.begin(), rsp_map.end());
      (*response->mutable_rsp_kv())["rsp-key-1"] = "rsp-value-1";
      (*response->mutable_rsp_kv())["rsp-key-2"] = "rsp-value-2";

      return ::trpc::kSuccStatus;
    } else if (request->test_case_type() == TransRequest::DIFFERENT_KEY_USE_ADD_INTERFACE) {
      context->AddRspTransInfo("rsp-key-1", "rsp-value-1");
      context->AddRspTransInfo("rsp-key-2", "rsp-value-2");
      (*response->mutable_rsp_kv())["rsp-key-1"] = "rsp-value-1";
      (*response->mutable_rsp_kv())["rsp-key-2"] = "rsp-value-2";

      return ::trpc::kSuccStatus;
    } else if (request->test_case_type() == TransRequest::SAME_KEY_USE_ADD_INTERFACE) {
      context->AddRspTransInfo("rsp-key-1", "rsp-value-1");
      context->AddRspTransInfo("rsp-key-1", "rsp-value-2");
      (*response->mutable_rsp_kv())["rsp-key-1"] = "rsp-value-2";

      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestAttachment(::trpc::ServerContextPtr context, const AttachmentRequest* request,
                                AttachmentResponse* response) {
    // inform the client about which messages the server has received
    response->set_recv_msg(request->msg());
    const ::trpc::NoncontiguousBuffer& req_atta = context->GetRequestAttachment();
    response->set_recv_atta(::trpc::FlattenSlow(req_atta));

    std::string rsp_str = "test_rsp";
    response->set_rsp_msg(rsp_str);
    if (request->test_case_type() == AttachmentRequest::NO_ATTACHMENT) {
      return ::trpc::kSuccStatus;
    } else if (request->test_case_type() == AttachmentRequest::NOT_EMPTY_ATTACHMENT) {
      context->SetResponseAttachment(::trpc::CreateBufferSlow(rsp_str.c_str(), rsp_str.size()));
      response->set_rsp_atta(rsp_str);
      return ::trpc::kSuccStatus;
    } else if (request->test_case_type() == AttachmentRequest::EMPTY_ATTACHMENT) {
      context->SetResponseAttachment(::trpc::NoncontiguousBuffer());
      return ::trpc::kSuccStatus;
    }
    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestTimeout(::trpc::ServerContextPtr context, const TimeoutRequest* request,
                             TimeoutResponse* response) {
    ::trpc::testing::SleepFor(request->server_deal_time());
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status QueryLastTimeoutInfo(::trpc::ServerContextPtr context, const QueryTimeoutRequest* request,
                                      QueryTimeoutResponse* response) {
    response->set_server_last_ret_code(last_ret_code);
    last_ret_code = 0;
    return ::trpc::kSuccStatus;
  }

 private:
  // the implementation of different testcases for compressor
  std::unordered_map<CompressorRequest::TestCaseType,
                     ::trpc::Function<::trpc::Status(::trpc::ServerContextPtr context, const CompressorRequest* request,
                                                     CompressorResponse* response)>>
      compressor_test_impl_map_;

  // the implementation of different testcases for pb serialization
  std::unordered_map<
      SerializationRequest::TestCaseType,
      ::trpc::Function<::trpc::Status(::trpc::ServerContextPtr context, const SerializationRequest* request,
                                      SerializationResponse* response)>>
      pb_serialization_test_impl_map_;
};

class FbsTestServiceImpl final : public FbsTest {
 public:
  ::trpc::Status TestFbs(const ::trpc::ServerContextPtr& context,
                         const ::flatbuffers::trpc::Message<FbsRequest>* request,
                         ::flatbuffers::trpc::Message<FbsResponse>* response) {
    ::flatbuffers::trpc::MessageBuilder mb;
    const FbsRequest* fb_request = request->GetRoot();

    if (fb_request->test_case_type() == FbsTestCaseType_FORMAL) {
      const std::string& req_msg = fb_request->message()->str();
      auto msg_offset = mb.CreateString(req_msg);
      auto reply_offset = CreateFbsResponse(mb, msg_offset);
      mb.Finish(reply_offset);
      *response = mb.ReleaseMessage<FbsResponse>();

      return ::trpc::kSuccStatus;
    } else if (fb_request->test_case_type() == FbsTestCaseType_INVALID_DATA) {
      // do not fill the response
      return ::trpc::kSuccStatus;
    } else if (fb_request->test_case_type() == FbsTestCaseType_INVALID_TYPE) {
      // use async respond to achieve the goal of changing the response data type
      context->SetResponse(false);
      std::string rsp_str = fb_request->message()->str();
      context->SetRspEncodeType(::trpc::serialization::kNoopType);
      context->SendUnaryResponse(::trpc::kSuccStatus, rsp_str);
    }
    return ::trpc::kUnknownErrorStatus;
  }
};

class JsonTestServiceImpl final : public ::trpc::RpcServiceImpl {
 public:
  JsonTestServiceImpl() {
    AddRpcServiceMethod(
        new ::trpc::RpcServiceMethod("/trpc.testing.unarytrpc.JsonTest/TestRapidJson", ::trpc::MethodType::UNARY,
                                     new ::trpc::RpcMethodHandler<::rapidjson::Document, ::rapidjson::Document>(
                                         std::bind(&JsonTestServiceImpl::TestRapidJson, this, std::placeholders::_1,
                                                   std::placeholders::_2, std::placeholders::_3))));
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        "/trpc.testing.unarytrpc.JsonTest/TestPbJson", ::trpc::MethodType::UNARY,
        new ::trpc::RpcMethodHandler<CommonRequest, CommonReply>(std::bind(&JsonTestServiceImpl::TestPbJson, this,
                                                                           std::placeholders::_1, std::placeholders::_2,
                                                                           std::placeholders::_3))));
  }

  trpc::Status TestRapidJson(::trpc::ServerContextPtr context, const ::rapidjson::Document* request,
                             ::rapidjson::Document* response) {
    if (request->IsObject() && request->HasMember("msg") && (*request)["msg"].IsString()) {
      std::string req_str = (*request)["msg"].GetString();
      std::string rsp_str = "{\"msg\":\"" + req_str + "\"}";
      response->Parse(rsp_str.c_str());
    }
    return ::trpc::kSuccStatus;
  }

  trpc::Status TestPbJson(::trpc::ServerContextPtr context, const CommonRequest* request, CommonReply* response) {
    context->SetRspEncodeType(::trpc::serialization::kJsonType);
    response->set_msg(request->msg());
    return ::trpc::kSuccStatus;
  }
};

class NoopTestServiceImpl final : public ::trpc::RpcServiceImpl {
 public:
  NoopTestServiceImpl() {
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        "/trpc.testing.unarytrpc.NoopTest/TestStringNoop", ::trpc::MethodType::UNARY,
        new ::trpc::RpcMethodHandler<std::string, std::string>(std::bind(&NoopTestServiceImpl::TestStringNoop, this,
                                                                         std::placeholders::_1, std::placeholders::_2,
                                                                         std::placeholders::_3))));
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        "/trpc.testing.unarytrpc.NoopTest/TestNonContiguousBufferNoop", ::trpc::MethodType::UNARY,
        new ::trpc::RpcMethodHandler<::trpc::NoncontiguousBuffer, ::trpc::NoncontiguousBuffer>(
            std::bind(&NoopTestServiceImpl::TestNonContiguousBufferNoop, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3))));
  }

  trpc::Status TestStringNoop(::trpc::ServerContextPtr context, const std::string* request, std::string* response) {
    *response = *request;
    return ::trpc::kSuccStatus;
  }

  trpc::Status TestNonContiguousBufferNoop(::trpc::ServerContextPtr context, const ::trpc::NoncontiguousBuffer* request,
                                           ::trpc::NoncontiguousBuffer* response) {
    context->SetRspEncodeType(::trpc::serialization::kNoopType);
    std::string req_str = ::trpc::FlattenSlow(*request);
    *response = ::trpc::CreateBufferSlow(req_str.c_str(), req_str.size());
    return ::trpc::kSuccStatus;
  }
};

class TrpcTransportServiceImpl : public trpc::testing::unarytrpc::TrpcTransport {
 public:
  ::trpc::Status TestCommon(::trpc::ServerContextPtr context, const CommonRequest* request, CommonReply* reply) {
    reply->set_msg(request->msg());
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestUdpResponse(::trpc::ServerContextPtr context, const CommonRequest* request, CommonReply* reply) {
    context->SetResponse(false);
    if (request->msg() == kUdpPacketOverLimit) {
      int maxUdpPacketSize = 65535;
      std::vector<char> rsp_msg;
      rsp_msg.assign(maxUdpPacketSize, 'a');
      ::trpc::NoncontiguousBufferBuilder builder;
      builder.Append(&rsp_msg[0], maxUdpPacketSize);
      ::trpc::NoncontiguousBuffer body = builder.DestructiveGet();
      context->SendUnaryResponse(trpc::kSuccStatus, body);
    } else {
      reply->set_msg("ok");
      context->SendUnaryResponse(trpc::kSuccStatus, *reply);
    }

    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestCustomDispatcher(::trpc::ServerContextPtr context, const CommonRequest* request,
                                      CommonReply* reply) {
    ::trpc::ThreadModel* thread_model = GetThreadModel();
    if (thread_model->Type() == ::trpc::kSeparate) {
      std::thread::id tid = std::this_thread::get_id();
      std::stringstream ss;
      ss << tid;
      reply->set_msg(ss.str());
    } else {
      reply->set_msg("0");
    }
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestIdleTimeout(::trpc::ServerContextPtr context, const CommonRequest* request, CommonReply* reply) {
    context->SetResponse(false);

    // delay response when connection idle timeout
    uint32_t delay_response_ms = atoi(request->msg().c_str());
    DelayExecute(
        [context]() {
          CommonReply reply;
          reply.set_msg("ok");
          context->SendUnaryResponse(trpc::kSuccStatus, reply);
        },
        delay_response_ms);

    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestDifferResponseWay(::trpc::ServerContextPtr context, const CommonRequest* request,
                                       CommonReply* reply) {
    auto req_msg = request->msg();
    if (req_msg == kSyncResponse) {
      reply->set_msg(req_msg);
    } else if (req_msg == kAsyncResponse) {
      context->SetResponse(false);
      reply->set_msg(req_msg);
      context->SendUnaryResponse(::trpc::kSuccStatus, *reply);
    } else if (req_msg == kNoResponse) {
      context->SetResponse(false);
    }

    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestDifferCallType(::trpc::ServerContextPtr context, const CommonRequest* request,
                                    CommonReply* reply) {
    auto req_msg = request->msg();
    if (req_msg == kOnewayRequest) {
      TRPC_ASSERT(context->GetCallType() == ::trpc::kOnewayCall);
    } else if (req_msg == kUnaryRequest) {
      TRPC_ASSERT(context->GetCallType() == ::trpc::kUnaryCall);
    }

    reply->set_msg(req_msg);
    return ::trpc::kSuccStatus;
  }

 private:
  void DelayExecute(::trpc::TimerExecutor&& executor, uint64_t delay_ms) {
    if (::trpc::runtime::IsInFiberRuntime()) {
      ::trpc::FiberSleepFor(std::chrono::milliseconds(delay_ms));
      executor();
    } else {
      auto timer = std::make_shared<::trpc::AsyncTimer>();
      timer->After(delay_ms).Then([timer, executor = std::move(executor)]() {
        executor();
        return trpc::MakeReadyFuture<bool>(true);
      });
    }
  }

  ::trpc::ThreadModel* GetThreadModel() {
    ::trpc::ThreadModel* thread_model = nullptr;
    if (::trpc::runtime::IsInFiberRuntime()) {
      thread_model = ::trpc::fiber::GetFiberThreadModel();
    } else {
      thread_model = ::trpc::merge::RandomGetMergeThreadModel();
      if (thread_model == nullptr) {
        thread_model = ::trpc::separate::RandomGetSeparateThreadModel();
      }
    }
    return thread_model;
  }
};

int TrpcTestServer::Initialize() {
  auto trpc_pb_service = std::make_shared<TrpcTestServiceImpl>();
  trpc_pb_service->SetServiceTimeoutHandleFunction(ServiceTimeoutFunc);
  RegisterService("trpc_test_service", trpc_pb_service);

  RegisterService("fbs_test_service", std::make_shared<FbsTestServiceImpl>());

  RegisterService("json_test_service", std::make_shared<JsonTestServiceImpl>());

  RegisterService("noop_test_service", std::make_shared<NoopTestServiceImpl>());

  auto transport_test_service = std::make_shared<TrpcTransportServiceImpl>();
  transport_test_service->SetHandleRequestDispatcherFunction([](const STransportReqMsg* req) {
    if (req->context && req->context->GetFuncName().find("TestCustomDispatcher") != std::string::npos) {
      return 0;
    }

    return -1;
  });
  RegisterService("trpc_transport_test_service", transport_test_service);

  // test uds only for separate threadmodel
  if (!::trpc::runtime::IsInFiberRuntime() && ::trpc::merge::RandomGetMergeThreadModel() == nullptr) {
    RegisterService("trpc_uds_transport_test_service", std::make_shared<TrpcTransportServiceImpl>());
  }

  server_->Start();

  test_signal_->SignalClientToContinue();

  return 0;
}

void TrpcTestServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
