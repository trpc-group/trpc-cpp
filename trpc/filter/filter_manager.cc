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

#include "trpc/filter/filter_manager.h"

#include "trpc/filter/client_filter_manager.h"
#include "trpc/filter/server_filter_manager.h"

namespace trpc {

bool FilterManager::AddMessageServerFilter(const MessageServerFilterPtr& filter) {
  return ServerFilterManager::GetInstance()->AddMessageServerFilter(filter);
}

MessageServerFilterPtr FilterManager::GetMessageServerFilter(const std::string& name) {
  return ServerFilterManager::GetInstance()->GetMessageServerFilter(name);
}

const std::unordered_map<std::string, MessageServerFilterPtr>& FilterManager::GetAllMessageServerFilters() {
  return ServerFilterManager::GetInstance()->GetAllMessageServerFilters();
}

bool FilterManager::AddMessageClientFilter(const MessageClientFilterPtr& filter) {
  return ClientFilterManager::GetInstance()->AddMessageClientFilter(filter);
}

MessageClientFilterPtr FilterManager::GetMessageClientFilter(const std::string& name) {
  return ClientFilterManager::GetInstance()->GetMessageClientFilter(name);
}

const std::unordered_map<std::string, MessageClientFilterPtr>& FilterManager::GetAllMessageClientFilters() {
  return ClientFilterManager::GetInstance()->GetAllMessageClientFilters();
}

bool FilterManager::AddMessageServerGlobalFilter(const MessageServerFilterPtr& filter) {
  return ServerFilterManager::GetInstance()->AddMessageServerGlobalFilter(filter);
}

const std::deque<MessageServerFilterPtr>& FilterManager::GetMessageServerGlobalFilter(FilterPoint type) {
  return ServerFilterManager::GetInstance()->GetMessageServerGlobalFilter(type);
}

bool FilterManager::AddMessageClientGlobalFilter(const MessageClientFilterPtr& filter) {
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  return ClientFilterManager::GetInstance()->AddMessageClientGlobalFilter(filter);
}

const std::deque<MessageClientFilterPtr>& FilterManager::GetMessageClientGlobalFilter(FilterPoint type) {
  return ClientFilterManager::GetInstance()->GetMessageClientGlobalFilter(type);
}

void FilterManager::Clear() {
  ClientFilterManager::GetInstance()->Clear();
  ServerFilterManager::GetInstance()->Clear();
}

}  // namespace trpc
