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

#include "trpc/admin/admin_service.h"

#include <unordered_set>

#include "trpc/admin/client_detach_handler.h"
#include "trpc/admin/commands_handler.h"
#include "trpc/admin/contention_profiler_handler.h"
#include "trpc/admin/cpu_profiler_handler.h"
#include "trpc/admin/heap_profiler_handler.h"
#include "trpc/admin/index_handler.h"
#include "trpc/admin/js_handler.h"
#include "trpc/admin/log_level_handler.h"
#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/admin/prometheus_handler.h"
#endif
#include "trpc/admin/reload_config_handler.h"
#include "trpc/admin/sample.h"
#include "trpc/admin/stats_handler.h"
#include "trpc/admin/sysvars_handler.h"
#include "trpc/admin/version_handler.h"
#include "trpc/admin/watch_handler.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

AdminService::AdminService() {
  auto commands_function =
      std::bind(&AdminService::ListCmds, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

  // Gets version.
  RegisterCmd(http::OperationType::GET, "/version", std::make_shared<admin::VersionHandler>());

  // Gets all supported admin commands.
  RegisterCmd(http::OperationType::GET, "/cmds", std::make_shared<admin::CommandsHandler>(commands_function));
  // Gets log level
  RegisterCmd(http::OperationType::GET, "/cmds/loglevel", std::make_shared<admin::LogLevelHandler>());
  // Updates log level.
  RegisterCmd(http::OperationType::PUT, "/cmds/loglevel", std::make_shared<admin::LogLevelHandler>(true));
  // Reloads config.
  RegisterCmd(http::OperationType::POST, "/cmds/reload-config", std::make_shared<admin::ReloadConfigHandler>());
  // Gets the watcher.
  RegisterCmd(http::OperationType::POST, "/cmds/watch", std::make_shared<admin::WatchHandler>());
  // Gets the stats
  RegisterCmd(http::OperationType::GET, "/cmds/stats", std::make_shared<admin::StatsHandler>());
  // Gets the vars.
  RegisterCmd(http::OperationType::GET, "/cmds/var", std::make_shared<admin::VarHandler>("/cmds/var"));
  RegisterCmd(http::OperationType::GET, "<regex(/cmds/var/.*)>", std::make_shared<admin::VarHandler>("/cmds/var"));
  // Extension for user defined vars.
  RegisterCmd(http::OperationType::GET, "/cmds/var/trpc", std::make_shared<admin::VarHandler>("/cmds/var"));
  RegisterCmd(http::OperationType::GET, "/cmds/var/user", std::make_shared<admin::VarHandler>("/cmds/var"));
#ifdef TRPC_BUILD_INCLUDE_RPCZ
  // Gets the rpcz.
  RegisterCmd(http::OperationType::GET, "/cmds/rpcz", std::make_shared<admin::RpczHandler>("/cmds/rpcz"));
  RegisterCmd(http::OperationType::GET, "<regex(/cmds/rpcz/.*)>", std::make_shared<admin::RpczHandler>("/cmds/rpcz"));
#endif
  // Gets the CPU profiling.
  RegisterCmd(http::OperationType::POST, "/cmds/profile/cpu", std::make_shared<admin::CpuProfilerHandler>());
  // Gets the heap profiling.
  RegisterCmd(http::OperationType::POST, "/cmds/profile/heap", std::make_shared<admin::HeapProfilerHandler>());

  ////////////////////////////////////////////// return html
  // Gets index.
  auto index_handler1 = std::make_shared<admin::IndexHandler>();
  auto index_handler2 = std::make_shared<admin::IndexHandler>();
  RegisterCmd(http::OperationType::GET, "", index_handler1);
  RegisterCmd(http::OperationType::GET, "/", index_handler2);

  // Static resource.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/js/jquery_min", std::make_shared<admin::JqueryJsHandler>());
  RegisterCmd(http::OperationType::GET, "/cmdsweb/js/viz_min", std::make_shared<admin::VizJsHandler>());
  RegisterCmd(http::OperationType::GET, "/cmdsweb/js/flot_min", std::make_shared<admin::FlotJsHandler>());

  // Gets the log info.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/logs", std::make_shared<admin::WebLogLevelHandler>());
  // Gets the stats.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/stats", std::make_shared<admin::WebStatsHandler>());
  // Gets the system vars.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/sysvars", std::make_shared<admin::WebSysVarsHandler>());
  // Gets the series data for vars.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/seriesvar", std::make_shared<admin::WebSeriesVarHandler>());

  // Gets the config.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/config", std::make_shared<admin::WebReloadConfigHandler>());
  // Gets the CPU profiling.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/profile/cpu", std::make_shared<admin::WebCpuProfilerHandler>());
  // Gets the CPU profiling draw.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/profile/cpu_draw",
              std::make_shared<admin::WebCpuProfilerDrawHandler>());
  // Gets the heap profiling.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/profile/heap", std::make_shared<admin::WebHeapProfilerHandler>());
  // Gets the heap profiling draw.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/profile/heap_draw",
              std::make_shared<admin::WebHeapProfilerDrawHandler>());
  // Gets the contention profiling.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/profile/contention",
              std::make_shared<admin::WebContentionProfilerHandler>());
  // Gets the contention profiling draw.
  RegisterCmd(http::OperationType::GET, "/cmdsweb/profile/contention_draw",
              std::make_shared<admin::WebContentionProfilerDrawHandler>());

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
  // Prometheus metrics.
  auto prometheus_handle_ptr = std::make_shared<admin::PrometheusHandler>();
  prometheus_handle_ptr->Init();
  RegisterCmd(http::OperationType::GET, "/metrics", prometheus_handle_ptr);
#endif

  RegisterCmd(http::OperationType::POST, "/client_detach", std::make_shared<admin::ClientDetachHandler>());

  StartSysvarsTask();
}

bool AdminService::BuildServiceAdapterOption(ServiceAdapterOption& option) {
  const ServerConfig& server_config = trpc::TrpcConfig::GetInstance()->GetServerConfig();
  option.service_name = "trpc." + server_config.app + "." + server_config.server + ".AdminService";
  option.network = "tcp";
  option.ip = server_config.admin_ip;
  option.is_ipv6 = false;
  option.port = atoi(server_config.admin_port.c_str());
  option.protocol = "http";
  option.queue_timeout = 5000;
  option.idle_time = server_config.admin_idle_time;
  option.timeout = UINT32_MAX;
  option.disable_request_timeout = false;
  option.max_conn_num = 100;
  option.max_packet_size = 10000000;
  option.threadmodel_instance_name = std::string(kSeparateAdminInstance);

  return (!option.ip.empty() && option.port > 0);
}

void AdminService::StartSysvarsTask() {
  admin::InitSystemVars();

  // Output a snapshot of resource utilization to file every 10 seconds.
  PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      []() {
        std::string sys_data = admin::GetBaseProfileDir() + std::string("sysvars.data");
        std::string html = "";
        admin::WebSysVarsHandler::PrintSysVarsData(&html);
        int ret = admin::WriteSysvarsData(sys_data.c_str(), html);
        if (ret) {
          TRPC_LOG_ERROR("WriteSysvarsData fail");
        }
      },
      10000, "AdminService WriteSysvarsData");

  // Updates resource utilization every 1 second.
  PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask([]() { admin::UpdateSystemVars(); }, 1000,
                                                                   "AdminService UpdateSystemVars");
}

void AdminService::RegisterCmd(http::OperationType type, const std::string path, AdminHandlerBase* handler) {
  unrepeatable_commands_.insert(path);
  routes_.Add(type, http::Path(path), std::shared_ptr<http::HandlerBase>(handler, [](auto*) {}));
}

void AdminService::RegisterCmd(http::OperationType type, const std::string& path,
                               const std::shared_ptr<http::HandlerBase>& handler) {
  unrepeatable_commands_.insert(path);
  routes_.Add(type, http::Path(path), handler);
}

void AdminService::ListCmds(http::HttpRequestPtr req, rapidjson::Value& result,
                            rapidjson::Document::AllocatorType& alloc) {
  rapidjson::Value operation_vec(rapidjson::kArrayType);
  for (auto const& cmd : unrepeatable_commands_) {
    if (cmd.empty() || cmd == "/" || cmd.find("cmdsweb") != std::string::npos ||
        cmd.find("<regex(") != std::string::npos) {
      continue;
    }
    operation_vec.PushBack(rapidjson::Value(cmd, alloc).Move(), alloc);
  }
  result.AddMember("cmds", operation_vec.Move(), alloc);
}

}  // namespace trpc
