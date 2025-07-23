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
#include <string>
#include <unordered_map>

#include "trpc/filter/filter.h"

namespace trpc {

/// @brief The filter management class, includes both server-side and client-side filters.
class FilterManager {
 public:
  static FilterManager* GetInstance() {
    static FilterManager instance;
    return &instance;
  }

  FilterManager(const FilterManager&) = delete;
  FilterManager& operator=(const FilterManager&) = delete;

  /// @brief Put the server filter into the server filter collection based on the filter name.
  bool AddMessageServerFilter(const MessageServerFilterPtr& filter);

  /// @brief Get the server filter by name.
  MessageServerFilterPtr GetMessageServerFilter(const std::string& name);

  /// @brief Get all server filters
  const std::unordered_map<std::string, MessageServerFilterPtr>& GetAllMessageServerFilters();

  /// @brief Put the client filter into the client filter collection based on the filter name.
  bool AddMessageClientFilter(const MessageClientFilterPtr& filter);

  /// @brief Get the client filter by name.
  MessageClientFilterPtr GetMessageClientFilter(const std::string& name);

  /// @brief Get all client filters
  const std::unordered_map<std::string, MessageClientFilterPtr>& GetAllMessageClientFilters();

  /// @brief Adding a global server filter, it will put the filter into the execution queue of each filter point
  ///        according to the calling order.
  /// @note The same filter can only be added once.
  bool AddMessageServerGlobalFilter(const MessageServerFilterPtr& filter);

  /// @brief Get global server filters by filter point.
  const std::deque<MessageServerFilterPtr>& GetMessageServerGlobalFilter(FilterPoint type);

  /// @brief Adding a global client filter, it will put the filter into the execution queue of each filter point
  ///        according to the calling order.
  /// @note The same filter can only be added once.
  bool AddMessageClientGlobalFilter(const MessageClientFilterPtr& filter);

  /// @brief Get global client filters by filter point.
  const std::deque<MessageClientFilterPtr>& GetMessageClientGlobalFilter(FilterPoint type);

  /// @brief Clean up the used resources.
  void Clear();

 private:
  FilterManager() = default;
};

}  // namespace trpc
