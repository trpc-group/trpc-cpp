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

#include "trpc/filter/client_filter_base.h"

namespace trpc {

/// @brief The client filter management class.
class ClientFilterManager {
 public:
  static ClientFilterManager* GetInstance() {
    static ClientFilterManager instance;
    return &instance;
  }

  ClientFilterManager(const ClientFilterManager&) = delete;
  ClientFilterManager& operator=(const ClientFilterManager&) = delete;

  /// @brief Put the client filter into the client filter collection based on the filter name.
  /// @return Return true on success, otherwise return false.
  bool AddMessageClientFilter(const MessageClientFilterPtr& filter);

  /// @brief Get client filter by name
  MessageClientFilterPtr GetMessageClientFilter(const std::string& name);

  /// @brief Get all client filters.
  const std::unordered_map<std::string, MessageClientFilterPtr>& GetAllMessageClientFilters();

  /// @brief Add a global client filter, it will put the filter into the execution queue of each filter point
  ///        according to the calling order.
  /// @note The same filter can only be added once.
  bool AddMessageClientGlobalFilter(const MessageClientFilterPtr& filter);

  /// @brief Get global client filters by filter point.
  const std::deque<MessageClientFilterPtr>& GetMessageClientGlobalFilter(FilterPoint type);

  /// @brief Clean up the used resources.
  void Clear();

 private:
  ClientFilterManager() = default;

  // Determine if a filter point is a post-point. The index parameter refers to the corresponding index of the point.
  bool PostOrder(int index) { return index & 0x1; }

  // Check if the filter points are paired. If they are paired, return true. If they are not paired, the current
  // implementation will assert and fail.
  bool CheckMatchPoints(std::vector<FilterPoint>& filter_points);

 private:
  // Container to store registered client filters, where the key is the filter name.
  std::unordered_map<std::string, MessageClientFilterPtr> message_client_filters_;

  // Queue array to store client global filters by filter point.
  std::deque<MessageClientFilterPtr> message_client_global_filters_[kFilterTypeNum];
};

}  // namespace trpc
