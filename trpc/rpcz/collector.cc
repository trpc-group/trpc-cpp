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
#include "trpc/rpcz/collector.h"

#include <algorithm>
#include <any>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/prctl.h>

#include "trpc/common/config/trpc_config.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/util/object_pool/object_pool.h"
#include "trpc/util/time.h"

namespace {

void DeleteSpan(trpc::rpcz::Span* span_ptr) {
  for (auto* sub_iter : span_ptr->SubSpans()) {
    DeleteSpan(sub_iter);
  }

  trpc::object_pool::Delete(span_ptr);
}

}  // namespace

namespace trpc::rpcz {

RpczCollectorTask::RpczCollectorTask() : start_(false), last_collect_time_(0), last_remove_time_(0) {
  rpcz_config_ = trpc::TrpcConfig::GetInstance()->GetGlobalConfig().rpcz_config;

  // To improve performance, cache_expire_interval not allowed to under kCacheExpireIntervalMin.
  if (rpcz_config_.cache_expire_interval < kCacheExpireIntervalMin) {
    TRPC_FMT_WARN("rpcz_config_.cache_expire_interval too small:{}", rpcz_config_.cache_expire_interval);
    rpcz_config_.cache_expire_interval = kCacheExpireIntervalMin;
  }

  TRPC_FMT_DEBUG("cache_expire_interval:{}, collect_interval_ms:{}, remove_interval_ms:{}",
                 rpcz_config_.cache_expire_interval, rpcz_config_.collect_interval_ms,
                 rpcz_config_.remove_interval_ms);
}

RpczCollectorTask::~RpczCollectorTask() { Destroy(); }

void RpczCollectorTask::Start() {
  if (!start_) {
    start_ = true;
    task_id_ = trpc::PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
        std::bind(&RpczCollectorTask::Run, this), 100, "RpczCollectorTask");
  }
}

void RpczCollectorTask::Destroy() {
  std::scoped_lock<std::mutex> lock(mutex_);
  for (auto iter = time_spans_.begin(); iter != time_spans_.end();) {
    for (Span* it : iter->second) {
      auto find_iter = span_id_spans_.find(it->SpanId());
      if (find_iter != span_id_spans_.end()) {
        span_id_spans_.erase(find_iter);
      }

      if (it != nullptr) {
        trpc::object_pool::Delete(it);
      }
    }
    iter = time_spans_.erase(iter);
  }
}

void RpczCollectorTask::Stop() {
  if (task_id_) {
    trpc::PeripheryTaskScheduler::GetInstance()->StopInnerTask(task_id_);
    task_id_ = 0;
  }
}

uint32_t RpczCollectorTask::ConvertUsToStoreTime(uint64_t us) {
  uint64_t second = us / (1000 * 1000);
  return second;
}

void RpczCollectorTask::OnCollect(PreprocessorSpanMap& processor_spans) {
  for (PreprocessorSpanMap::iterator it = processor_spans.begin(); it != processor_spans.end(); ++it) {
    it->second.clear();
  }

  LinkNode<Span>* head = RpczCollector::GetInstance()->Reset();
  if (!head) {
    TRPC_FMT_TRACE("TLS has not Span");
    return;
  }

  LinkNode<Span> tmp_root;
  head->InsertBeforeAsList(&tmp_root);
  head = nullptr;

  SpanPreprocessor* prep = SpanPreprocessor::GetInstance();
  for (LinkNode<Span>* p = tmp_root.next(); p != &tmp_root;) {
    LinkNode<Span>* saved_next = p->next();
    p->RemoveFromList();
    processor_spans[prep].push_back(p->value());
    p = saved_next;
  }

  LinkNode<Span> root;
  for (PreprocessorSpanMap::iterator it = processor_spans.begin(); it != processor_spans.end(); ++it) {
    std::vector<Span*>& list = it->second;
    if (it->second.empty()) {
      continue;
    }

    if (it->first != nullptr) {
      it->first->process(list);
    }

    for (size_t i = 0; i < list.size(); ++i) {
      Span* rpcz_span = list[i];
      uint32_t span_id = rpcz_span->SpanId();
      uint64_t first_log = rpcz_span->FirstLogRealUs();
      // Special for kSpanTypeUser.
      if (rpcz_span->Type() == SpanType::kSpanTypeUser)
        first_log = rpcz_span->BeginViewerEvent().timestamp_us;

      uint32_t store_time = ConvertUsToStoreTime(first_log);

      {
        std::scoped_lock<std::mutex> lock(mutex_);
        span_id_spans_.emplace(span_id, rpcz_span);
        if (time_spans_.find(store_time) == time_spans_.end()) {
          std::vector<Span*> store_time_spans{rpcz_span};
          time_spans_[store_time] = store_time_spans;
        } else {
          time_spans_[store_time].emplace_back(rpcz_span);
        }
      }
    }
  }
}

void RpczCollectorTask::OnRemove() {
  std::scoped_lock<std::mutex> lock(mutex_);

  uint32_t current_time = ConvertUsToStoreTime(trpc::GetSystemMicroSeconds());
  for (auto iter = time_spans_.begin(); iter != time_spans_.end();) {
    if (iter->first + rpcz_config_.cache_expire_interval <= current_time) {
      for (Span* it : iter->second) {
        auto find_iter = span_id_spans_.find(it->SpanId());
        if (find_iter != span_id_spans_.end()) {
          span_id_spans_.erase(find_iter);
        }

        if (it != nullptr) {
          DeleteSpan(it);
        }
      }
      iter = time_spans_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void RpczCollectorTask::Run() {
  uint64_t now_ms = trpc::time::GetMilliSeconds();
  if (now_ms >= last_collect_time_ + rpcz_config_.collect_interval_ms) {
    OnCollect(processor_spans_);
    last_collect_time_ = now_ms;
  }

  if (now_ms >= last_remove_time_ + rpcz_config_.remove_interval_ms) {
    OnRemove();
    last_remove_time_ = now_ms;
  }
  TRPC_FMT_TRACE("RpczCollectorTask Running");
}

void RpczCollector::Start() { collect_task_.Start(); }

void RpczCollector::Stop() { collect_task_.Stop(); }

void RpczCollector::Submit(std::any data) {
  Span* span = std::any_cast<Span*>(data);
  if (span != nullptr) {
    *this << span;
  }
}

void RpczCollector::FillServerBriefSpanInfo(const Span& span, std::ostream& brief_span_info) {
  std::string span_type;
  uint64_t first_log = span.FirstLogRealUs();
  uint64_t cost = span.LastLogRealUs() - span.FirstLogRealUs();
  if (span.Type() == SpanType::kSpanTypeServer) {
    span_type = "S";
  } else if (span.Type() == SpanType::kSpanTypeUser) {
    span_type = "U";
    // Special for kSpanTypeUser.
    first_log = span.BeginViewerEvent().timestamp_us;
    cost = span.EndViewerEvent().timestamp_us - span.BeginViewerEvent().timestamp_us;
  } else {
    span_type = "C";
  }

  std::string framwork_ret_info;
  if (span.ErrorCode() == 0) {
    framwork_ret_info = "ok";
  } else {
    framwork_ret_info = "failed";
  }

  brief_span_info << trpc::TimeStringHelper::ConvertMicroSecsToStr(first_log)
                  << "   cost=" << cost << "(us)"
                  << " span_type=" << span_type << " span_id=" << span.SpanId() << " " << span.FullMethodName()
                  << " request=" << span.RequestSize() << " response=" << span.ResponseSize() << " ["
                  << framwork_ret_info << "]" << std::endl;
}

bool RpczCollector::FillRpczData(const std::any& params, std::string& rpcz_data) {
  RpczAdminReqData admin_req = std::any_cast<const RpczAdminReqData&>(params);
  std::stringstream span_info;

  SpanIdSpanMap span_id_spans = collect_task_.GetLocalSpanIdSpanData();

  // Get general info.
  if (admin_req.params_size == 0) {
    uint32_t print_num = 0;
    uint32_t print_spans_num = trpc::TrpcConfig::GetInstance()->GetGlobalConfig().rpcz_config.print_spans_num;
    for (auto iter = span_id_spans.begin(); (iter != span_id_spans.end()) && (print_num < print_spans_num); iter++) {
      FillServerBriefSpanInfo(*(iter->second), span_info);
      print_num++;
    }
    rpcz_data = span_info.str();
    return true;
  }

  auto find_iter = span_id_spans.find(admin_req.span_id);
  if (find_iter != span_id_spans.end()) {
    if (find_iter->second->Type() == SpanType::kSpanTypeServer) {
      auto info = find_iter->second->ServerSpanToString();
      rpcz_data = std::move(info);
    } else if (find_iter->second->Type() == SpanType::kSpanTypeClient) {
      auto info = find_iter->second->ClientSpanToString();
      rpcz_data = std::move(info);
    } else if (find_iter->second->Type() == SpanType::kSpanTypeUser) {
      auto info = find_iter->second->UserSpanToString();
      rpcz_data = std::move(info);
    }
    return true;
  } else {
    TRPC_FMT_ERROR("rpcz span_id is not found, plase check it");
    return false;
  }
}

}  // namespace trpc::rpcz
#endif
