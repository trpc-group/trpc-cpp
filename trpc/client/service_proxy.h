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

#include <any>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "trpc/client/client_context.h"
#include "trpc/client/service_proxy_option.h"
#include "trpc/codec/client_codec.h"
#include "trpc/common/future/future.h"
#include "trpc/common/status.h"
#include "trpc/filter/client_filter_controller.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/stream/stream.h"
#include "trpc/transport/client/client_transport.h"
#include "trpc/transport/client/preallocation_option.h"
#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/util/unique_id.h"

namespace trpc {

/// @brief Base class of service proxy.
class ServiceProxy {
 public:
  virtual ~ServiceProxy() = default;

  /// @brief Synchronous call interface oriented to transport (thread-safe).
  /// @param req Protocol object with serialized request body data.
  /// @param rsp Protocol object with body data not yet serialized.
  /// @return OK status on success, otherwise on failed.
  /// @note The implementation of this interface is thread-blocking in IO/Handle separation or merging mode,
  ///       and non-thread-blocking in Fiber mode.
  Status UnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp);

  /// @brief Asynchronous call interface oriented to transport (thread-safe).
  /// @param req Protocol object with serialized request body data.
  /// @return Ready future with protocol object with body data not yet serialized on success, failed future on failed.
  Future<ProtocolPtr> AsyncUnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req);

  /// @brief One-way call interface oriented to transport (thread-safe).
  /// @return OK status on success, otherwise on failed.
  /// @note It only completes request sending without waiting for response.
  Status OnewayInvoke(const ClientContextPtr& context, const ProtocolPtr& req);

  /// @brief Get option of service proxy (thread-safe).
  const ServiceProxyOption* GetServiceProxyOption() { return option_.get(); }
  ServiceProxyOption* GetMutableServiceProxyOption() { return option_.get(); }

  /// @brief Get codec used by service proxy (thread-safe).
  const ClientCodecPtr& GetClientCodec() { return codec_; }

  /// @brief Stop used resources by service proxy (thread-safe).
  virtual void Stop();

  /// @brief Destroy used resources by service proxy (thread-safe).
  virtual void Destroy();

  /// @brief Pre-allocate a connection, used for scenarios that require fixed connections for data transmission
  /// (thread-safe).
  /// @param[in] preallocate_option Pre-allocated parameters related to backend nodes.
  /// @return std::nullopt on failed, connection identifier on success.
  std::optional<uint64_t> GetOrCreateConnector(const PreallocationOption& preallocate_option);

  /// @brief Release a fixed connection (not thread-safe).
  /// @param[in] connector_id connection identifier
  /// @return true on success，false on failed.
  /// @note When releasing, it is necessary to ensure that there is no data interaction on the current fixed connection.
  ///       The same connection can only be released once. If the release fails, it can be retried.
  bool ReleaseFixedConnector(uint64_t connector_id);

  /// @brief Initiate disconnection from all connections with a specified IP node.
  /// @param[in] target_ip The IP address of the specified node.
  /// @note Currently, only the IO/Handle separation and merging mode is supported.
  void Disconnect(const std::string& target_ip);

 protected:
  /// @brief Synchronous call interface oriented to transport, it will be call by UnaryInvoke.
  virtual void UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp);

  /// @brief Asynchronous call interface oriented to transport, it will be call by AsyncUnaryInvoke.
  virtual Future<ProtocolPtr> AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                        const ProtocolPtr& req_protocol);

  /// @brief One-way call interface oriented to transport, it will be call by OnewayInvoke.
  virtual void OnewayTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req);

  /// @brief Create StreamReaderWriterProvider
  virtual stream::StreamReaderWriterProviderPtr SelectStreamProvider(const ClientContextPtr& context,
                                                                     void* rpc_reply_msg = nullptr);

  /// @brief Whether a selector plugin is required for the proxy.
  /// @note If a selector is not required for the subclass, it can be overridden to return false.
  virtual bool NeedSelector() { return true; }

  /// @brief Set the options of service proxy, which can be overridden by the subclass to customize the settings.
  virtual void SetServiceProxyOptionInner(const std::shared_ptr<ServiceProxyOption>& option);

  /// @brief Used to obtain transinfo based on options, which can be overridden by the subclass to customize the
  ///        trans_info.
  virtual TransInfo ProxyOptionToTransInfo();

  /// @brief Return the service routing name, which is the name of service proxy for direct connections, and the target
  ///        in the options for non-direct connection models.
  const std::string& GetServiceName();

  // Collect statistics on the service proxy and report them to the tvar.
  void ProxyStatistics(const ClientContextPtr& ctx);

  // Used to determine if the request has timed out. Returns true if timed out, false if normal.
  bool CheckTimeout(const ClientContextPtr& context);

  /// @brief Use the proxy option parameters to set the context.
  void FillClientContext(const ClientContextPtr& context);

  /// @brief Run filters by filter point.
  int RunFilters(const FilterPoint& point, const ClientContextPtr& context);

  /// @brief Get the threadmodel used by service proxy.
  ThreadModel* GetThreadModel();

  /// @brief Init transport used by service proxy. By default, the framework uses its built-in transport.
  /// @note Subclasses can implement transport by overriding this method themselves.
  virtual void InitTransport();

  /// @brief Init the derived class members if need. It will be called after SetServiceProxyOptionInner.
  virtual void InitOtherMembers();

 private:
  // Init the service routing name by config
  void InitServiceNameInfo();

  // Check if the tvar variable required for statistical has been created, and create it if it has not been created.
  void PrepareStatistics(const std::string& service_name);

  // Determine if pipeline is supported.
  bool SupportPipeline(const std::shared_ptr<ServiceProxyOption>& option);

  // Add a service-level filter.
  void AddServiceFilter(const MessageClientFilterPtr& filter);

  // Set the codec plugin to be used.
  void SetCodec(const std::string& codec_name);

  // Execute client-side scheduling tracking.
  FilterStatus RunClientFilters(FilterPoint filter_point, CTransportReqMsg* msg);

  // Execute IO tracking for rpcz.
  FilterStatus RunIoFilters(const FilterPoint& point, const std::any& msg) noexcept;

  // Routing selection interface, retrieve IP and port based on service name and store in ctx.
  bool SelectTarget(const ClientContextPtr& context);

  // Get connection type by config
  ConnectionType GetClientConnectType();

  // Init the used filters.
  void InitFilters();

  // Init the used selector filter, to ensure compatibility with filters that use selector_name to specify the selector
  // plugin.
  void InitSelectorFilter();

  // Error handling, it will set error information to context.
  void HandleError(const ClientContextPtr& context, int ret, std::string&& err_msg);

  // Set the host/port for direct connection to the service, used when the selector is 'domain' or 'direct'.
  // endpoint_info is a comma-separated list of host and port. The supported formats are as follows:
  // 1. ipv4:port, as: 127.0.0.1:90
  // 2. [ipv6]:port, as: [::1]:90
  // 3. domain:port(only applicable when the selector is 'domain'), as: www.qq.com:8080
  void SetEndpointInfo(const std::string& endpoint_info);

 protected:
  ClientCodecPtr codec_;

  std::unique_ptr<ClientTransport> transport_{nullptr};

  ClientFilterController filter_controller_;

 private:
  std::shared_ptr<ServiceProxyOption> option_;

  UniqueId unique_id_;

  // service routing name
  std::string service_name_;

  ThreadModel* thread_model_{nullptr};

  // Total count of backup requests retries at the service level.
  std::shared_ptr<tvar::Counter<uint64_t>> backup_retries_{nullptr};

  // Count of successful backup request retries at the service level.
  std::shared_ptr<tvar::Counter<uint64_t>> backup_retries_succ_{nullptr};

  friend class ServiceProxyManager;
};

using ServiceProxyPtr = std::shared_ptr<ServiceProxy>;

}  // namespace trpc
