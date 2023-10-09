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

#include "trpc/util/log/logging.h"

#include <iostream>
#include <vector>

#include "trpc/common/config/config_helper.h"
#include "trpc/util/log/default/default_log.h"

namespace trpc::log {

bool Init() {
  std::vector<std::string> log_names;
  if (!ConfigHelper::GetInstance()->GetNodes({"plugins", "log"}, log_names)) {
    std::cout << "logging plugin configurations is not setted, log will be printed to the console." << std::endl;
    return true;
  }
  // The log plugin is not configured.
  if (log_names.empty()) {
    std::cout << "default logging plugin configuration is not setted, log will be printed to console." << std::endl;
    return true;
  }
  // Make sure the configuration is "plugins-log-default-xxx".
  if (log_names.size() > 1 || std::strcmp(log_names[0].c_str(), kTrpcLogCacheStringDefault) != 0) {
    return false;
  }
  // "default" enabled plugins.
  DefaultLogPtr p = MakeRefCounted<DefaultLog>();
  int ret = p->Init();
  if (ret != 0) {
    std::cerr << "Log name: " << log_names[0] << ", init failed." << std::endl;
    exit(-1);
  }
  p->Start();
  // The log plug-in is registered with the log factory.
  LogFactory::GetInstance()->Register(p);

  return true;
}

void Destroy() {
  auto log = LogFactory::GetInstance()->Get();

  if (log) {
    LogFactory::GetInstance()->Reset();

    log->Stop();
    log->Destroy();
  }
}

bool IsLogOn(const char* instance_name, trpc::Log::Level level) {
  const auto& p = trpc::LogFactory::GetInstance()->Get();
  if (p) return p->ShouldLog(instance_name, level);
  return false;
}

}  // namespace trpc::log
