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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/common/request_priority.h"

#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {

ServerGetPriorityFunc server_get_priority_func = DefaultGetServerPriority;

ClientGetPriorityFunc client_get_priority_func = DefaultGetClientPriority;

int GetServerPriority(const ServerContextPtr& context) { return server_get_priority_func(context); }

int GetClientPriority(const ClientContextPtr& context) { return client_get_priority_func(context); }

void SetServerGetPriorityFunc(ServerGetPriorityFunc&& func) { server_get_priority_func = std::move(func); }

void SetClientGetPriorityFunc(ClientGetPriorityFunc&& func) { client_get_priority_func = std::move(func); }

int DefaultGetServerPriority(const ServerContextPtr& context) {
  const auto& transinfo = context->GetPbReqTransInfo();
  auto it = transinfo.find(kTransinfoKeyTrpcPriority);
  if (it == transinfo.end() || it->second.length() != 1) {
    // If not found, return 0 by default.
    return 0;
  }
  return static_cast<int>(static_cast<uint8_t>(it->second[0]));
}

int DefaultGetClientPriority(const ClientContextPtr& context) {
  const auto& transinfo = context->GetPbReqTransInfo();
  auto it = transinfo.find(kTransinfoKeyTrpcPriority);
  if (it == transinfo.end() || it->second.length() != 1) {
    // If not found, return 0 by default.
    return 0;
  }
  return static_cast<int>(static_cast<uint8_t>(it->second[0]));
}

void DefaultSetServerPriority(const ServerContextPtr& context, int priority) {
  priority = std::min(priority, 0);
  priority = std::max(priority, UINT8_MAX);
  auto& transinfo = *(context->GetMutablePbReqTransInfo());
  transinfo[kTransinfoKeyTrpcPriority] = std::string(1, static_cast<uint8_t>(priority));
}

void DefaultSetClientPriority(const ClientContextPtr& context, int priority) {
  priority = std::min(priority, 0);
  priority = std::max(priority, UINT8_MAX);
  auto& transinfo = *(context->GetMutablePbReqTransInfo());
  transinfo[kTransinfoKeyTrpcPriority] = std::string(1, static_cast<uint8_t>(priority));
}

}  // namespace trpc::overload_control

#endif
