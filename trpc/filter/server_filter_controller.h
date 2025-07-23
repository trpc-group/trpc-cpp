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

#include <deque>
#include <vector>

#include "trpc/filter/server_filter_base.h"

namespace trpc {

/// @brief Execution controller for server filters.
class ServerFilterController {
 public:
  /// @brief Run server filters in specified fitler point.
  /// @param type filter point
  /// @param context server context
  /// @return Return CONTINUE on success, otherwise return REJECT.
  /// @note The global filters are executed first, followed by the service-level filter.
  FilterStatus RunMessageServerFilters(FilterPoint type, const ServerContextPtr& context);

  /// @brief Add a service-level's server filter.
  /// @return On success return true, otherwise return false.
  /// @note The same filter can only be added once.
  bool AddMessageServerFilter(const MessageServerFilterPtr& filter);

 private:
  // Determine if a filter point is a post-point.
  inline bool PostOrder(int index) { return index & 0x1; }

 private:
  // Queue array to store service-level's server filters by filter point.
  std::deque<MessageServerFilterPtr> server_filters_[kFilterTypeNum];
};

}  // namespace trpc
