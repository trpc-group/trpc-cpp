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

#include <memory>
#include <string>
#include <vector>

#include "trpc/client/client_context.h"
#include "trpc/filter/client_filter_base.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/selector_factory.h"

namespace trpc {

/// @brief Route selection and call result reporting functions are encapsulated for
// service discovery filter of the framework to call
class SelectorWorkFlow {
 public:
  SelectorWorkFlow(const std::string& plugin_name, bool need_report, bool has_metadata)
      : plugin_name_(plugin_name), need_report_(need_report), has_metadata_(has_metadata) {
    selector_ = SelectorFactory::GetInstance()->Get(plugin_name);
  }

  ~SelectorWorkFlow() = default;

  /// @brief Initializes the SelectorWorkFlow object.
  int Init();

  /// @brief Executes the corresponding processing at the specified filter point.
  /// @param status The execution result.
  /// @param point The filter point.
  /// @param context The client context.
  void RunFilter(FilterStatus& status, FilterPoint point, const ClientContextPtr& context);

  /// @brief Synchronously selects a routing node for a service and stores the result in the client context.
  /// @param context The client context.
  /// @return bool Returns true if a routing node was successfully selected, false otherwise.
  bool SelectTarget(const ClientContextPtr& context);

  /// @brief Reports the result of a service invocation to trigger circuit breaking.
  /// @param context The client context.
  /// @return int Returns 0 on success, -1 on failure.
  int ReportInvokeResult(const ClientContextPtr& context);

  /// @brief Sets the function for getting the name of the service being called.
  /// @param func The function for getting the service name.
  void SetGetServiceNameFunc(Function<const std::string&(const ClientContextPtr& context)>&& func) {
    get_service_name_func_ = std::move(func);
  }

 private:
  // Gets the name of the service being called.
  const std::string& GetServiceName(const ClientContextPtr& context);

  // Fills the SelectorInfo object with information about the service being called.
  void FillSelectorInfo(const ClientContextPtr& context, SelectorInfo& selector_info);

  // set request addr of peer server
  void SetRequestAddr(const ClientContextPtr& context, TrpcEndpointInfo&& instance);

  // Set addrs used for backup request
  void SetBackupRequestAddrs(const ClientContextPtr& context, std::vector<TrpcEndpointInfo>&& instances);

  // Set the actual node of backup request to client context
  void SetActualBackupRequestNode(const ClientContextPtr& context);

  // Fills the InvokeResult object with information about the service invocation.
  void FillInvokeResult(const ClientContextPtr& context, InvokeResult& result);

  // Determines whether to report the service invocation result based on the framework return code.
  bool ShouldReport(int framework_retcode);

 private:
  // The name of the selector plugin.
  std::string plugin_name_;
  // The selector plugin instance.
  SelectorPtr selector_;
  // Whether to report the service invocation result to trigger circuit breaking.
  bool need_report_;
  // Whether to fill the metadata for the selected service endpoint.
  bool has_metadata_;

  // The function for getting the name of the service being called.
  Function<const std::string&(const ClientContextPtr& context)> get_service_name_func_ = nullptr;
};

}  // namespace trpc
