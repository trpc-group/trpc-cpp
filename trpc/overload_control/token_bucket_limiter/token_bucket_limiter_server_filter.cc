/*
 *
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2024 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_server_filter.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/log/trpc_log.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/overload_control/common/report.h"
#include "trpc/runtime/common/stats/frame_stats.h"

namespace trpc::overload_control {

int TokenBucketLimiterServerFilter::Init() {
  bool ok = TrpcConfig::GetInstance()->GetPluginConfig<TokenBucketLimiterControlConf>(
      kOverloadCtrConfField, kTokenBucketLimiterName, token_bucket_conf_);
  if (!ok) {
    TRPC_FMT_DEBUG("TokenBucketLimiterServerFilter read config failed, will use a default config");
  }
  token_bucket_conf_.Display();

  service_controller_ = std::make_shared<TokenBucketOverloadController>();
  service_controller_->Register(token_bucket_conf_);
  service_controller_->Init();

  return 0;
}

std::vector<FilterPoint> TokenBucketLimiterServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // This tracking point is not used, but tracking points must be paired, so it is added here.
      FilterPoint::SERVER_POST_SCHED_RECV_MSG,
  };
}

void TokenBucketLimiterServerFilter::operator()(FilterStatus& status, FilterPoint point,
                                                const ServerContextPtr& context) {
  switch (point) {
    case FilterPoint::SERVER_PRE_SCHED_RECV_MSG: {
      OnRequest(status, context);
      break;
    }
    default: {
      break;
    }
  }
}

void TokenBucketLimiterServerFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is already a dirty request, it will not be processed further to ensure that the first error code is
    // not overwritten.
    return;
  }

  auto now = trpc::time::GetSystemMicroSeconds();
  bool passed = service_controller_->BeforeSchedule(context);
  if (!passed) {
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server token bucket limiter overload control"));
    status = FilterStatus::REJECT;
  }

  // Report the result.
  if (token_bucket_conf_.is_report) {
    auto remaining_tokens{service_controller_->GetRemainingTokens(now)};
    OverloadInfo infos;
    infos.attr_name = kOverloadctrlTokenBucketLimiter;
    infos.report_name = fmt::format("/{}/{}", context->GetCalleeName(), context->GetFuncName());
    infos.tags[kOverloadctrlPass] = (passed == true ? 1 : 0);
    infos.tags[kOverloadctrlLimited] = (passed == false ? 1 : 0);
    infos.tags["burst"] = service_controller_->GetBurst();
    infos.tags["remaining_tokens"] = remaining_tokens;
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
}

}  // namespace trpc::overload_control

#endif
