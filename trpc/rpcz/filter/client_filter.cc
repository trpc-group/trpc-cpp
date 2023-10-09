//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/filter/client_filter.h"

#include "trpc/filter/filter_point.h"
#include "trpc/rpcz/collector.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/rpcz/rpcz.h"
#include "trpc/rpcz/util/sampler.h"
#include "trpc/util/string_util.h"
#include "trpc/util/time.h"

namespace trpc::rpcz {

RpczClientFilter::RpczClientFilter() : customer_func_set_flag_(false) {
  high_low_water_level_sampler_.Init();

  ClientFilterPointHandleFunction pre_rpc_handle =
      std::bind(&RpczClientFilter::PreRpcHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction pre_send_handle =
      std::bind(&RpczClientFilter::PreSendHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction pre_sched_send_handle =
      std::bind(&RpczClientFilter::PreSchedSendHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction post_sched_send_handle =
      std::bind(&RpczClientFilter::PostSchedSendHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction post_io_send_handle =
      std::bind(&RpczClientFilter::PostIoSendHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction pre_sched_recv_handle =
      std::bind(&RpczClientFilter::PreSchedRecvHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction post_sched_recv_handle =
      std::bind(&RpczClientFilter::PostSchedRecvHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction post_recv_handle =
      std::bind(&RpczClientFilter::PostRecvHandle, this, std::placeholders::_1);
  ClientFilterPointHandleFunction post_rpc_handle =
      std::bind(&RpczClientFilter::PostRpcHandle, this, std::placeholders::_1);

  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE, pre_rpc_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_PRE_SEND_MSG, pre_send_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_PRE_SCHED_SEND_MSG, pre_sched_send_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_POST_SCHED_SEND_MSG, post_sched_send_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_POST_IO_SEND_MSG, post_io_send_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, pre_sched_recv_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_POST_SCHED_RECV_MSG, post_sched_recv_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_POST_RECV_MSG, post_recv_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::CLIENT_POST_RPC_INVOKE, post_rpc_handle));
}

std::vector<trpc::FilterPoint> RpczClientFilter::GetFilterPoint() {
  std::vector<trpc::FilterPoint> points = {
      trpc::FilterPoint::CLIENT_PRE_SEND_MSG,       trpc::FilterPoint::CLIENT_POST_RECV_MSG,
      trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE,     trpc::FilterPoint::CLIENT_POST_RPC_INVOKE,
      trpc::FilterPoint::CLIENT_PRE_SCHED_SEND_MSG, trpc::FilterPoint::CLIENT_POST_SCHED_SEND_MSG,
      trpc::FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, trpc::FilterPoint::CLIENT_POST_SCHED_RECV_MSG,
      trpc::FilterPoint::CLIENT_PRE_IO_SEND_MSG,    trpc::FilterPoint::CLIENT_POST_IO_SEND_MSG};
  return points;
}

void RpczClientFilter::FillContextToRouteSpan(const trpc::ClientContextPtr& context, Span* span) {
  span->SetProtocolName(context->GetCodecName());
  span->SetSpanType(SpanType::kSpanTypeClient);

  // full_method_name = /trpc.xx.xx.xx/Sayhello
  std::string full_method_name = context->GetFuncName();
  if (full_method_name.size() > 1) {
    span->SetFullMethodName(full_method_name.substr(1));
  } else {
    TRPC_FMT_ERROR("full_method_name:{} format is error", full_method_name);
  }
  span->SetRemoteName(context->GetCalleeName());
  uint64_t now_us = trpc::GetSystemMicroSeconds();
  span->SetFirstLogRealUs(now_us);
  span->SetStartRpcInvokeRealUs(now_us);
}


bool RpczClientFilter::HighLowWaterSample(uint32_t span_id) { return high_low_water_level_sampler_.Sample(span_id); }

bool RpczClientFilter::ShouldSample(const trpc::ClientContextPtr& context, uint32_t span_id) {
  if (customer_func_set_flag_) {
    return customer_rpcz_sample_function_(context);
  }

  return HighLowWaterSample(span_id);
}

void RpczClientFilter::PreRpcHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag) {
      Span* span_ptr = trpc::object_pool::New<Span>();
      ptr->span = span_ptr;
      FillContextToRouteSpan(context, ptr->span);
    }
    return;
  }

  // Pure client scenario.
  ClientRpczSpan client_rpcz_span;
  uint32_t span_id = SpanIdGenerator::GetInstance()->GenerateId();
  client_rpcz_span.sample_flag = ShouldSample(context, span_id);
  if (client_rpcz_span.sample_flag) {
    Span* span_ptr = trpc::rpcz::CreateClientRpczSpan(context, span_id);
    client_rpcz_span.span = span_ptr;
  } else {
    client_rpcz_span.span = nullptr;
  }
  context->SetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex, std::move(client_rpcz_span));
}

void RpczClientFilter::PreSendHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag) {
      ptr->span->SetStartTransInvokeRealUs(trpc::GetSystemMicroSeconds());
      ptr->span->SetTimeout(context->GetTimeout());
      std::string ip_port;
      ip_port.append(context->GetIp());
      ip_port.append(":");
      ip_port.append(trpc::util::Convert<std::string, int>(context->GetPort()));
      ptr->span->SetRemoteSide(ip_port);
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr) {
    client_rpcz_span_ptr->span->SetStartTransInvokeRealUs(trpc::GetSystemMicroSeconds());

    std::string ip_port;
    ip_port.append(context->GetIp());
    ip_port.append(":");
    ip_port.append(trpc::util::Convert<std::string, int>(context->GetPort()));
    client_rpcz_span_ptr->span->SetRemoteSide(ip_port);
  }
}

void RpczClientFilter::PreSchedSendHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag && context->GetRequest() != nullptr) {
      ptr->span->SetRequestSize(context->GetRequest()->GetMessageSize());
      ptr->span->SetStartSendRealUs(trpc::GetSystemMicroSeconds());
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr &&
      context->GetRequest() != nullptr) {
    client_rpcz_span_ptr->span->SetRequestSize(context->GetRequest()->GetMessageSize());
    client_rpcz_span_ptr->span->SetStartSendRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczClientFilter::PostSchedSendHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag) {
      ptr->span->SetSendRealUs(trpc::GetSystemMicroSeconds());
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr) {
    client_rpcz_span_ptr->span->SetSendRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczClientFilter::PostIoSendHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag) {
      ptr->span->SetSendDoneRealUs(trpc::GetSystemMicroSeconds());
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr) {
    client_rpcz_span_ptr->span->SetSendDoneRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczClientFilter::PreSchedRecvHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag) {
      ptr->span->SetStartHandleRealUs(trpc::GetSystemMicroSeconds());
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr) {
    client_rpcz_span_ptr->span->SetStartHandleRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczClientFilter::PostSchedRecvHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag) {
      ptr->span->SetHandleRealUs(trpc::GetSystemMicroSeconds());
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr) {
    client_rpcz_span_ptr->span->SetHandleRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczClientFilter::PostRecvHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag) {
      ptr->span->SetTransInvokeDoneRealUs(trpc::GetSystemMicroSeconds());
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr) {
    client_rpcz_span_ptr->span->SetTransInvokeDoneRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczClientFilter::PostRpcHandle(const trpc::ClientContextPtr& context) {
  // Route scenario.
  auto* ptr = context->GetFilterData<ClientRouteRpczSpan>(kTrpcRouteRpczIndex);
  if (ptr) {
    if (ptr->sample_flag && ptr->parent_span) {
      uint64_t now_us = trpc::GetSystemMicroSeconds();
      ptr->span->SetRpcInvokeDoneRealUs(now_us);
      ptr->span->SetErrorCode(context->GetStatus().GetFrameworkRetCode());
      ptr->span->SetResponseSize(context->GetResponseLength());
      ptr->span->SetLastLogRealUs(now_us);
      ptr->parent_span->AppendSubSpan(ptr->span);
    }
    return;
  }

  // Pure client scenario.
  auto* client_rpcz_span_ptr = context->GetFilterData<ClientRpczSpan>(kTrpcClientRpczIndex);
  if (client_rpcz_span_ptr && client_rpcz_span_ptr->sample_flag && client_rpcz_span_ptr->span != nullptr) {
    uint64_t now_us = trpc::GetSystemMicroSeconds();
    client_rpcz_span_ptr->span->SetRpcInvokeDoneRealUs(now_us);
    client_rpcz_span_ptr->span->SetErrorCode(context->GetStatus().GetFrameworkRetCode());
    client_rpcz_span_ptr->span->SetResponseSize(context->GetResponseLength());
    client_rpcz_span_ptr->span->SetLastLogRealUs(now_us);
    RpczCollector::GetInstance()->Submit(client_rpcz_span_ptr->span);
  }
}

void RpczClientFilter::operator()(trpc::FilterStatus& status, trpc::FilterPoint point,
                                  const trpc::ClientContextPtr& context) {
  auto it = point_handles_.find(point);
  if (it == point_handles_.end()) {
    status = trpc::FilterStatus::CONTINUE;
    return;
  }
  it->second(context);
  status = trpc::FilterStatus::CONTINUE;
}

void RpczClientFilter::SetSampleFunction(const CustomerClientRpczSampleFunction& sample_function) {
  customer_func_set_flag_ = true;
  customer_rpcz_sample_function_ = sample_function;
}

void RpczClientFilter::DelSampleFunction() {
  customer_func_set_flag_ = false;
  customer_rpcz_sample_function_ = nullptr;
}

}  // namespace trpc::rpcz
#endif
