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
#include <vector>

#include "trpc/filter/server_filter_base.h"

namespace trpc {

/// @brief The server filter management class.
class ServerFilterManager {
 public:
  static ServerFilterManager* GetInstance() {
    static ServerFilterManager instance;
    return &instance;
  }

  ServerFilterManager(const ServerFilterManager&) = delete;
  ServerFilterManager& operator=(const ServerFilterManager&) = delete;

  /// @brief Put the server filter into the server filter collection based on the filter name.
  /// @return Return true on success, otherwise return false.
  bool AddMessageServerFilter(const MessageServerFilterPtr& filter);

  /// @brief Get the server filter by name.
  MessageServerFilterPtr GetMessageServerFilter(const std::string& name);

  /// @brief Get all server filters.
  const std::unordered_map<std::string, MessageServerFilterPtr>& GetAllMessageServerFilters();

  /// @brief Add a global server filter, it will put the filter into the execution queue of each filter point
  ///        according to the calling order.
  /// @note The same filter can only be added once.
  bool AddMessageServerGlobalFilter(const MessageServerFilterPtr& filter);

  /// @brief Get global server filters by filter point.
  const std::deque<MessageServerFilterPtr>& GetMessageServerGlobalFilter(FilterPoint type);

  /// @brief Clean up the used resources.
  void Clear();

 private:
  ServerFilterManager() = default;

  // Determine if a filter point is a post-point. The index parameter refers to the corresponding index of the point.
  inline bool PostOrder(int index) { return index & 0x1; }

  // Check if the filter points are paired. If they are paired, return true. If they are not paired, the current
  // implementation will assert and fail.
  bool CheckMatchPoints(std::vector<FilterPoint>& filter_points);

 private:
  // Container to store registered server filters, where the key is the filter name.
  std::unordered_map<std::string, MessageServerFilterPtr> message_server_filters_;

  // Queue array to store global server filters by filter point.
  std::deque<MessageServerFilterPtr> message_server_global_filters_[kFilterTypeNum];
};

}  // namespace trpc
