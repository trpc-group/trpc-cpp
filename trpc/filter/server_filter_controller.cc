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

#include "trpc/filter/server_filter_controller.h"

#include "trpc/filter/server_filter_manager.h"
#include "trpc/util/log/logging.h"
#include "trpc/server/server_context.h"

namespace trpc {

bool ServerFilterController::AddMessageServerFilter(const MessageServerFilterPtr& filter) {
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  // Check for duplicate additions.
  if (points.size() > 0) {
    int point = static_cast<int>(points[0]) & kServerFilterMask;

    // Check if the filter is in the global filter. If it exists, it will return false.
    const std::deque<MessageServerFilterPtr>& server_global_filters =
        ServerFilterManager::GetInstance()->GetMessageServerGlobalFilter(points[0]);
    for (auto& item : server_global_filters) {
      if (!(item->Name().compare(filter->Name()))) {
        return false;
      }
    }

    // Check if the filter is in the service-level filter.
    for (auto& item : server_filters_[point]) {
      if (!(item->Name().compare(filter->Name()))) {
        return false;
      }
    }
  }

  // push the filter to queue
  for (size_t i = 0; i < points.size(); ++i) {
    int point = static_cast<int>(points[i]) & kServerFilterMask;
    if (PostOrder(point)) {
      server_filters_[point].push_front(filter);
    } else {
      server_filters_[point].push_back(filter);
    }
  }

  return true;
}

FilterStatus ServerFilterController::RunMessageServerFilters(FilterPoint type, const ServerContextPtr& context) {
  TRPC_ASSERT(context);
  FilterStatus status = FilterStatus::CONTINUE;
  int point = static_cast<int>(type) & kServerFilterMask;
  // execute global filters first
  if (!PostOrder(point)) {
    auto& global_filters = ServerFilterManager::GetInstance()->GetMessageServerGlobalFilter(type);
    for (uint32_t i = 0; i < global_filters.size(); ++i) {
      global_filters[i]->operator()(status, type, context);
      if (status == FilterStatus::REJECT) {
        // execute filter fail, save the index of current filter (the index of the global filter is the index in the
        // global_filters array).
        context->SetFilterExecIndex(type, i);
        return status;
      }
    }

    // execute service-level filter
    auto& service_filters = server_filters_[point];
    for (uint32_t i = 0; i < service_filters.size(); ++i) {
      service_filters[i]->operator()(status, type, context);
      if (status == FilterStatus::REJECT) {
        // execute filter fail, save the index of current service-level filter (the index is the index in
        // the service_filters array plus the size of global filters).
        context->SetFilterExecIndex(type, i + global_filters.size());
        return status;
      }
    }
  } else {
    auto exec_index = context->GetFilterExecIndex(GetMatchPoint(type));
    auto& global_filters = ServerFilterManager::GetInstance()->GetMessageServerGlobalFilter(type);
    auto& service_filters = server_filters_[point];

    if (exec_index == -1) {
      exec_index = global_filters.size() + service_filters.size();
    }

    // The execution order of post-point is opposite to that of pre-point.
    // The service-level filters are executed first, followed by the global filters.
    // To ensure that all post-filters of successfully executed filters can be executed, post-filters do not need to
    // check whether the previous filters have been executed successfully.
    if (static_cast<uint32_t>(exec_index) > global_filters.size()) {
      // calculate the offset of the service filters that need to be executed
      auto index = service_filters.size() - (exec_index - global_filters.size());
      for (uint32_t i = index; i < service_filters.size(); ++i) {
        service_filters[i]->operator()(status, type, context);
      }
      exec_index = global_filters.size();
    }

    // calculate the offset of the global filters that need to be executed
    auto index = global_filters.size() - exec_index;
    for (uint32_t i = index; i < global_filters.size(); ++i) {
      global_filters[i]->operator()(status, type, context);
    }
  }

  return status;
}

}  // namespace trpc
