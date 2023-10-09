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

#pragma once

#include <deque>
#include <vector>

#include "trpc/filter/client_filter_base.h"

namespace trpc {

/// @brief Execution controller for client filters.
class ClientFilterController {
 public:
  /// @brief Run client filters in specified fitler point.
  /// @param type filter point
  /// @param context client context
  /// @return Return CONTINUE on success, otherwise return REJECT.
  /// @note The global filters are executed first, followed by the service-level filter.
  FilterStatus RunMessageClientFilters(FilterPoint type, const ClientContextPtr& context);

  /// @brief Add a service-level's client filter.
  /// @return On success return true, otherwise return false.
  /// @note The same filter can only be added once.
  bool AddMessageClientFilter(const MessageClientFilterPtr& filter);

 private:
  // Determine if a filter point is a post-point.
  inline bool PostOrder(int index) { return index & 0x1; }

 private:
  // Queue array to store service-level's client filters by filter point.
  std::deque<MessageClientFilterPtr> client_filters_[kFilterTypeNum];
};

}  // namespace trpc
