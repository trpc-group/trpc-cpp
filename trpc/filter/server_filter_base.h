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

#include <any>

#include "trpc/common/status.h"
#include "trpc/filter/filter_base.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class ServerContext;

using ServerContextPtr = RefPtr<ServerContext>;
using MessageServerFilter = Filter<ServerContextPtr>;
using MessageServerFilterPtr = std::shared_ptr<MessageServerFilter>;

/// @brief Abstract class for server-side filters.
template <>
class Filter<ServerContextPtr> {
 public:
  Filter() { filter_id_ = GetNextFilterID(); }

  virtual ~Filter() = default;

  /// @brief Name of filter
  /// @return String type name
  virtual std::string Name() = 0;

  /// @brief Initialize the filter.
  /// @return Return zero if initialize successfully, otherwise return non-zero.
  virtual int Init() { return 0; }

  /// @brief Get the collection of tracking points
  /// @return Vector object containing tracking points.
  virtual std::vector<FilterPoint> GetFilterPoint() = 0;

  /// @brief Execute the logic corresponding to the tracking point.
  /// @param [out] status Store result of execution status.
  /// @param [in] point Buried point.
  /// @param context Context of server
  virtual void operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) = 0;

  /// @brief Retrieve the filter ID stored in filter_id_ member variable
  /// @return The filter ID associated with the filter instance
  virtual uint16_t GetFilterID() { return filter_id_; }

  /// @brief The method is used to create filter, which can be overloaded to implement data binding between each service
  ///        and its service-level filters.
  ///        That means each service has its own independent service filter, and does not share this filter object with
  ///        other services.
  /// @param param The parameters used for create filter
  virtual MessageServerFilterPtr Create(const std::any& param) { return nullptr; }

 private:
  // Note that filter_id is allocated in the range [10000, 65535).
  uint16_t filter_id_ = 10000;
};

}  // namespace trpc
