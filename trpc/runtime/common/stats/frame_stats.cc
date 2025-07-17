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

#include "trpc/runtime/common/stats/frame_stats.h"

#include <functional>

#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

void FrameStats::Start() {
  // Get metrics plugins
  TrpcConfig::GetInstance()->GetPluginNodes("metrics", metrics_);

  // start periodical task
  if (task_id_ == 0) {
    last_server_stats_time_ = trpc::time::GetMilliSeconds();
    TRPC_LOG_DEBUG("FrameStats task start");
    task_id_ = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(std::bind(&FrameStats::Run, this), 10,
                                                                                "FrameStats");
  }
}

void FrameStats::Stop() {
  if (task_id_) {
    PeripheryTaskScheduler::GetInstance()->StopInnerTask(task_id_);
    PeripheryTaskScheduler::GetInstance()->JoinInnerTask(task_id_);
    task_id_ = 0;
  }
}

void FrameStats::Run() {
  auto now_ms = trpc::time::GetMilliSeconds();

  if (TrpcConfig::GetInstance()->GetServerConfig().server_stats_interval > 0 &&
      TrpcConfig::GetInstance()->GetServerConfig().enable_server_stats) {
    // Print the statistical of server
    if (last_server_stats_time_ + TrpcConfig::GetInstance()->GetServerConfig().server_stats_interval <= now_ms) {
      server_stats_.Stats();
      last_server_stats_time_ = now_ms;
    }
  }

  // Report the statistical of backup request
  if (last_backup_request_time_ + BackupRequestStats::kBackupRequestIntervalMs <= now_ms) {
    for (auto& metrics_name : metrics_) {
      backup_request_stats_.AsyncReportToMetrics(metrics_name);
    }
    last_backup_request_time_ = now_ms;
  }

  TRPC_LOG_TRACE("FrameStats Running");
}

}  // namespace trpc
