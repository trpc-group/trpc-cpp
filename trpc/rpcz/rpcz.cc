
//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/rpcz.h"

#include <memory>
#include <ostream>
#include <sstream>

#include "trpc/common/config/trpc_config.h"
#include "trpc/rpcz/collector.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_util.h"
#include "trpc/util/time.h"

namespace trpc::rpcz {

std::string FormatFuncNameIfPossible(const std::string& func_name) {
  // Remove leading slash.
  if (func_name.empty()) {
    TRPC_LOG_ERROR("Error format empty function name");
    return "";
  }
  if (func_name.length() == 1) {
    return func_name;
  }
  if (func_name.find_first_of("/") == 0) {
    return func_name.substr(1);
  }
  return func_name;
}

Span* CreateServerRpczSpan(const trpc::ServerContextPtr& context,
                           uint32_t req_seq_id,
                           const uint64_t recv_timestamp_us) {
  Span* span_ptr = trpc::object_pool::New<Span>();
  if (!span_ptr) {
    TRPC_FMT_ERROR("create span failed");
    return nullptr;
  }

  span_ptr->SetTraceId(0);
  span_ptr->SetSpanId(req_seq_id);
  span_ptr->SetReceivedRealUs(recv_timestamp_us);
  span_ptr->SetCallType(static_cast<uint32_t>(context->GetCallType()));

  std::string ip_port;
  ip_port.append(context->GetIp());
  ip_port.append(":");
  ip_port.append(trpc::util::Convert<std::string, int>(context->GetPort()));
  span_ptr->SetRemoteSide(ip_port);

  span_ptr->SetProtocolName(context->GetCodecName());
  span_ptr->SetRequestSize(context->GetRequestLength());
  span_ptr->SetSpanType(SpanType::kSpanTypeServer);
  uint64_t now_us = trpc::GetSystemMicroSeconds();
  span_ptr->SetFirstLogRealUs(recv_timestamp_us);
  span_ptr->SetStartHandleRealUs(now_us);
  span_ptr->SetFullMethodName(FormatFuncNameIfPossible(context->GetFuncName()));

  span_ptr->SetRemoteName(context->GetCallerName());
  return span_ptr;
}

Span* CreateClientRpczSpan(const trpc::ClientContextPtr& context, uint32_t req_seq_id) {
  Span* span_ptr = trpc::object_pool::New<Span>();
  if (!span_ptr) {
    TRPC_FMT_ERROR("create span failed");
    return nullptr;
  }
  span_ptr->SetTraceId(0);
  span_ptr->SetSpanId(req_seq_id);
  span_ptr->SetCallType(static_cast<uint32_t>(context->GetCallType()));
  span_ptr->SetProtocolName(context->GetCodecName());

  span_ptr->SetSpanType(SpanType::kSpanTypeClient);
  uint64_t now_us = trpc::GetSystemMicroSeconds();
  span_ptr->SetFirstLogRealUs(now_us);
  span_ptr->SetStartRpcInvokeRealUs(now_us);
  span_ptr->SetFullMethodName(FormatFuncNameIfPossible(context->GetFuncName()));
  span_ptr->SetRemoteName(context->GetCalleeName());
  return span_ptr;
}

Span* CreateUserRpczSpan(const std::string& viewer_name) {
  Span* span_ptr = trpc::object_pool::New<Span>(viewer_name);
  if (!span_ptr) {
    TRPC_FMT_ERROR("create span failed");
    return nullptr;
  }

  uint32_t span_id = SpanIdGenerator::GetInstance()->GenerateId();
  span_ptr->SetSpanId(span_id);

  return span_ptr;
}

Span* CreateUserRpczSpan(const std::string& viewer_name, trpc::ServerContextPtr& context) {
  Span* span_ptr = trpc::object_pool::New<Span>(viewer_name);
  if (!span_ptr) {
    TRPC_FMT_ERROR("create span failed");
    return nullptr;
  }

  uint32_t span_id = SpanIdGenerator::GetInstance()->GenerateId();
  span_ptr->SetSpanId(span_id);

  UserRpczSpan user_rpcz_span;
  user_rpcz_span.span = span_ptr;
  context->SetFilterData<UserRpczSpan>(kTrpcUserRpczIndex, std::move(user_rpcz_span));

  return span_ptr;
}

Span* GetUserRpczSpan(const trpc::ServerContextPtr& context) {
  UserRpczSpan* ptr = context->GetFilterData<UserRpczSpan>(kTrpcUserRpczIndex);

  if (!ptr) return nullptr;

  return ptr->span;
}

void SubmitUserRpczSpan(Span* root_span_ptr) {
  root_span_ptr->End();
  RpczCollector::GetInstance()->Submit(root_span_ptr);
}

std::string TryGet(const trpc::http::HttpRequestPtr& req) {
  RpczAdminReqData req_params;
  req_params.span_id = trpc::util::Convert<uint32_t>(req->GetQueryParameter("span_id"));
  req_params.params_size = req->GetQueryParameters().FlatPairsCount();
  std::string rpcz_data;
  RpczCollector::GetInstance()->FillRpczData(req_params, rpcz_data);
  return rpcz_data;
}

void RpczPrint(const trpc::ServerContextPtr& context,
               const char* filename_in,
               int line_in,
               const std::string_view& msg) {
  if (!context) {
    TRPC_FMT_ERROR("context is nullptr, cant exe RpczPrint");
    return;
  }

  ServerRpczSpan* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->span) {
    uint64_t now_us = trpc::GetSystemMicroSeconds();
    uint64_t cost_time = 0;
    if (ptr->span->SubSpans().size() == 0) {
      cost_time = now_us - ptr->span->StartCallbackRealUs();
    } else {
      cost_time = now_us - ptr->span->SubSpans().back()->LastLogRealUs();
    }

    std::string now_time = trpc::TimeStringHelper::ConvertMicroSecsToStr(now_us);
    std::stringstream os;
    os << now_time << "   " << cost_time << "(us) [" << filename_in << ":" << line_in << "] " << msg.data()
       << std::endl;

    Span* sub_span = trpc::object_pool::New<Span>();
    sub_span->SetLastLogRealUs(now_us);
    sub_span->SetSpanType(trpc::rpcz::SpanType::kSpanTypePrint);
    sub_span->AppendCustomLogs(std::move(os.str()));
    ptr->span->AppendSubSpan(sub_span);
  }
}

}  // namespace trpc::rpcz
#endif
