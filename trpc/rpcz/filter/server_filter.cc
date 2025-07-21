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
#include "trpc/rpcz/filter/server_filter.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_point.h"
#include "trpc/rpcz/collector.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/rpcz/rpcz.h"
#include "trpc/rpcz/util/sampler.h"
#include "trpc/transport/common/transport_message.h"
#include "trpc/util/time.h"

namespace trpc::rpcz {

RpczServerFilter::RpczServerFilter() : customer_func_set_flag_(false) {
  high_low_water_level_sampler_.Init();

  ServerFilterPointHandleFunction pre_sched_recv_handle =
      std::bind(&RpczServerFilter::PreSchedRecvHandle, this, std::placeholders::_1);
  ServerFilterPointHandleFunction post_sched_recv_handle =
      std::bind(&RpczServerFilter::PostSchedRecvHandle, this, std::placeholders::_1);
  ServerFilterPointHandleFunction pre_rpc_handle =
      std::bind(&RpczServerFilter::PreRpcHandle, this, std::placeholders::_1);
  ServerFilterPointHandleFunction post_rpc_handle =
      std::bind(&RpczServerFilter::PostRpcHandle, this, std::placeholders::_1);
  ServerFilterPointHandleFunction pre_send_handle =
      std::bind(&RpczServerFilter::PreSendHandle, this, std::placeholders::_1);
  ServerFilterPointHandleFunction pre_sched_send_handle =
      std::bind(&RpczServerFilter::PreSchedSendHandle, this, std::placeholders::_1);
  ServerFilterPointHandleFunction post_sched_send_handle =
      std::bind(&RpczServerFilter::PostSchedSendHandle, this, std::placeholders::_1);
  ServerFilterPointHandleFunction post_io_send_handle =
      std::bind(&RpczServerFilter::PostIoSendHandle, this, std::placeholders::_1);

  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_PRE_SCHED_RECV_MSG, pre_sched_recv_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_POST_SCHED_RECV_MSG, post_sched_recv_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_PRE_RPC_INVOKE, pre_rpc_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_POST_RPC_INVOKE, post_rpc_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_PRE_SEND_MSG, pre_send_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_PRE_SCHED_SEND_MSG, pre_sched_send_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_POST_SCHED_SEND_MSG, post_sched_send_handle));
  point_handles_.insert(std::make_pair(trpc::FilterPoint::SERVER_POST_IO_SEND_MSG, post_io_send_handle));
}

std::vector<trpc::FilterPoint> RpczServerFilter::GetFilterPoint() {
  std::vector<trpc::FilterPoint> points = {
      trpc::FilterPoint::SERVER_POST_RECV_MSG,      trpc::FilterPoint::SERVER_PRE_SEND_MSG,
      trpc::FilterPoint::SERVER_PRE_SCHED_RECV_MSG, trpc::FilterPoint::SERVER_POST_SCHED_RECV_MSG,
      trpc::FilterPoint::SERVER_PRE_RPC_INVOKE,     trpc::FilterPoint::SERVER_POST_RPC_INVOKE,
      trpc::FilterPoint::SERVER_PRE_SCHED_SEND_MSG, trpc::FilterPoint::SERVER_POST_SCHED_SEND_MSG,
      trpc::FilterPoint::SERVER_PRE_IO_SEND_MSG,    trpc::FilterPoint::SERVER_POST_IO_SEND_MSG};
  return points;
}

bool RpczServerFilter::HighLowWaterSample(uint32_t span_id) { return high_low_water_level_sampler_.Sample(span_id); }

bool RpczServerFilter::ShouldSample(const trpc::ServerContextPtr& context, uint32_t span_id) {
  if (customer_func_set_flag_) {
    return customer_rpcz_sample_function_(context);
  }

  return HighLowWaterSample(span_id);
}

void RpczServerFilter::PreSchedRecvHandle(const trpc::ServerContextPtr& context) {
  ServerRpczSpan svr_rpcz_span;
  uint32_t span_id = SpanIdGenerator::GetInstance()->GenerateId();
  svr_rpcz_span.sample_flag = ShouldSample(context, span_id);

  if (svr_rpcz_span.sample_flag) {
    // convert timestamp from steady_clock to timestamp from system_clock
    uint64_t epoch_timestamp =
        trpc::time::GetSystemMicroSeconds() - (trpc::time::GetSteadyMicroSeconds() - context->GetRecvTimestampUs());
    Span* span_ptr = trpc::rpcz::CreateServerRpczSpan(context, span_id, epoch_timestamp);
    svr_rpcz_span.span = span_ptr;
  } else {
    svr_rpcz_span.span = nullptr;
  }

  context->SetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex, std::move(svr_rpcz_span));
}

void RpczServerFilter::PostSchedRecvHandle(const trpc::ServerContextPtr& context) {
  auto* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->sample_flag && ptr->span != nullptr) {
    ptr->span->SetHandleRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczServerFilter::PreRpcHandle(const trpc::ServerContextPtr& context) {
  auto* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->sample_flag && ptr->span != nullptr) {
    ptr->span->SetStartCallbackRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczServerFilter::PostRpcHandle(const trpc::ServerContextPtr& context) {
  auto* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->sample_flag && ptr->span != nullptr) {
    ptr->span->SetCallbackRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczServerFilter::PreSendHandle(const trpc::ServerContextPtr& context) {
  auto* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->sample_flag && ptr->span != nullptr) {
    ptr->span->SetStartEncodeRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczServerFilter::PreSchedSendHandle(const trpc::ServerContextPtr& context) {
  auto* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->sample_flag && ptr->span != nullptr) {
    ptr->span->SetStartSendRealUs(trpc::GetSystemMicroSeconds());
  }
}

void RpczServerFilter::PostSchedSendHandle(const trpc::ServerContextPtr& context) {
  auto* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->sample_flag && ptr->span != nullptr) {
    ptr->span->SetSendRealUs(trpc::GetSystemMicroSeconds());
    ptr->span->SetErrorCode(context->GetStatus().GetFrameworkRetCode());
    if (context->GetResponseMsg()) {
      ptr->span->SetResponseSize(context->GetResponseMsg()->GetMessageSize());
    }
  }
}

void RpczServerFilter::PostIoSendHandle(const trpc::ServerContextPtr& context) {
  auto* ptr = context->GetFilterData<ServerRpczSpan>(kTrpcServerRpczIndex);
  if (ptr && ptr->sample_flag && ptr->span != nullptr) {
    auto now_us = trpc::GetSystemMicroSeconds();
    ptr->span->SetSendDoneRealUs(now_us);
    ptr->span->SetLastLogRealUs(now_us);
    RpczCollector::GetInstance()->Submit(ptr->span);
  }
}

void RpczServerFilter::operator()(trpc::FilterStatus& status, trpc::FilterPoint point,
                                  const trpc::ServerContextPtr& context) {
  auto it = point_handles_.find(point);
  if (it == point_handles_.end()) {
    status = trpc::FilterStatus::CONTINUE;
    return;
  }
  it->second(context);
  status = trpc::FilterStatus::CONTINUE;
}

void RpczServerFilter::SetSampleFunction(const CustomerServerRpczSampleFunction& sample_function) {
  customer_func_set_flag_ = true;
  customer_rpcz_sample_function_ = sample_function;
}

void RpczServerFilter::DelSampleFunction() {
  customer_func_set_flag_ = false;
  customer_rpcz_sample_function_ = nullptr;
}

}  // namespace trpc::rpcz
#endif
