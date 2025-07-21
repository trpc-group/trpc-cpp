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

#include "trpc/filter/client_filter_controller.h"
#include "trpc/filter/server_filter_controller.h"

namespace trpc {

/// @brief Execution controller for filters.
class FilterController {
 public:
  /// @brief Run server filters with specified filter point
  FilterStatus RunMessageServerFilters(FilterPoint type, const ServerContextPtr& context);

  /// @brief Run client filters with specified filter point
  FilterStatus RunMessageClientFilters(FilterPoint type, const ClientContextPtr& context);

  /// @brief Add a service-level's server filter.
  /// @return On success return true, otherwise return false.
  /// @note The same filter can only be added once.
  bool AddMessageServerFilter(const MessageServerFilterPtr& filter);

  /// @brief Add a service-level's client filter.
  /// @return On success return true, otherwise return false.
  /// @note The same filter can only be added once.
  bool AddMessageClientFilter(const MessageClientFilterPtr& filter);

 private:
  ServerFilterController server_filter_controller_;

  ClientFilterController client_filter_controller_;
};

}  // namespace trpc
