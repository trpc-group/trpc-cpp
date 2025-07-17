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

#pragma once

namespace trpc::runtime {

/// @brief Start the runtime information reporting task. Currently reported information includes:
///        CPU utilization, TCP connection count, CPU core count, process PID, process file descriptor count, 
///        memory usage, total number of threads in process, number of tasks in task queue, streaming statistical data.
/// @note  To use this feature, you need to configure the metrics plugin and turn on the feature configuration switch.
///        An configuration example is as follows:
///        global:
///          enable_runtime_report: true  # the default value is true
///        ...
///        plugins:
///          metrics:
///            xxx:
void StartReportRuntimeInfo();

/// @brief Stop the runtime information reporting task.
void StopReportRuntimeInfo();

}  // namespace trpc::runtime
