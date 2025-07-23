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

#include "trpc/filter/client_filter_manager.h"

#include <algorithm>
#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc {

bool ClientFilterManager::CheckMatchPoints(std::vector<FilterPoint>& filter_points) {
  for (auto& point : filter_points) {
    auto match_point = GetMatchPoint(point);
    auto iter = std::find(filter_points.begin(), filter_points.end(), match_point);
    if (iter == filter_points.end()) {
      TRPC_LOG_ERROR("Filter points is wrong, need to add match point");
      TRPC_ASSERT(0 && "Filter points is wrong, need to add match point");
      return false;
    }
  }

  return true;
}

bool ClientFilterManager::AddMessageClientFilter(const MessageClientFilterPtr& filter) {
  auto filter_points = filter->GetFilterPoint();
  if (CheckMatchPoints(filter_points)) {
    message_client_filters_[filter->Name()] = filter;
    return true;
  }

  return false;
}

MessageClientFilterPtr ClientFilterManager::GetMessageClientFilter(const std::string& name) {
  auto iter = message_client_filters_.find(name);
  if (iter != message_client_filters_.end()) {
    return iter->second;
  }

  return nullptr;
}

const std::unordered_map<std::string, MessageClientFilterPtr>& ClientFilterManager::GetAllMessageClientFilters() {
  return message_client_filters_;
}

bool ClientFilterManager::AddMessageClientGlobalFilter(const MessageClientFilterPtr& filter) {
  std::vector<FilterPoint> points = filter->GetFilterPoint();

  // Check for duplicate additions.
  if (points.size() > 0) {
    int point = static_cast<int>(points[0]);
    // Check if the filter is in the global filter. If it exists, it will return false.
    const std::deque<MessageClientFilterPtr>& client_global_filters = message_client_global_filters_[point];
    for (auto& item : client_global_filters) {
      if (!(item->Name().compare(filter->Name()))) {
        return false;
      }
    }
  }

  for (size_t i = 0; i < points.size(); ++i) {
    int point = static_cast<int>(points[i]);
    if (PostOrder(point)) {
      message_client_global_filters_[point].push_front(filter);
    } else {
      message_client_global_filters_[point].push_back(filter);
    }
  }

  return true;
}

const std::deque<MessageClientFilterPtr>& ClientFilterManager::GetMessageClientGlobalFilter(FilterPoint type) {
  int point = static_cast<int>(type);
  return message_client_global_filters_[point];
}

void ClientFilterManager::Clear() {
  // clear global client filters
  for (int i = 0; i < kFilterTypeNum; ++i) {
    message_client_global_filters_[i].clear();
  }

  // clear client filter
  message_client_filters_.clear();
}

}  // namespace trpc
