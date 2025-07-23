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
#include <unordered_map>
#include <utility>

#include "trpc/admin/admin_service.h"
#include "trpc/common/config/server_conf.h"
#include "trpc/naming/trpc_naming.h"
#include "trpc/server/service_adapter.h"
#include "trpc/transport/server/server_transport.h"

namespace trpc {

/// @brief The function type that stops the server
using ServerTerminateFunction = std::function<bool()>;

/// @brief Implementation of trpc server
/// If used independently, the usage is as follows：
///
/// std::shared_ptr<TrpcServer> server = GetTrpcServer();
/// server->Initialize();
/// server->SetTerminateFunction(...);
/// //...
/// server->RegisterService(...);
/// //...
/// server->Start();
/// server->WaitForShutdown();
/// server->Destroy();
///
class TrpcServer {
 public:
  explicit TrpcServer(const ServerConfig& server_config);

  /// @brief Initialization
  bool Initialize();

  /// @brief Start and run
  bool Start();

  /// @brief Stop and wait for finish
  void WaitForShutdown();

  /// @brief Destroy internal resources of the server
  void Destroy();

  /// @brief Set the function object for service termination
  void SetTerminateFunction(ServerTerminateFunction terminate_function) {
    terminate_function_ = std::move(terminate_function);
  }

  /// @brief Error code of RegistryService interface returns
  enum class RegisterRetCode : int {
    /// success
    kOk = 0,
    /// illegal parameter
    kParameterError = 1,
    /// service name invalid
    kSearchError = 2,
    /// service self-registration failed
    kRegisterError = 3,
    /// unknown error
    kUnknownError = 4,
  };

  /// @brief The state of server
  enum class ServerState : int {
    kUnInitialize = 0,
    kInitialized = 1,
    kStart = 2,
    kShutDown = 3,
    kDestroy = 4,
  };

  /// @brief Register service by configuration
  /// @param service_name service name, must be unique
  /// @param service service object
  /// @param auto_start whether to automatically start and service
  /// @return RegisterRetCode
  /// @note  the premise of this method is that the service configuration
  ///        corresponding to the service name has been configured
  ///        in the framework configuration `/server/service`
  ///        if not, it will register failed
  ///        if you don't want to use this configuration method to register the service
  ///        use `RegisterService` by code
  RegisterRetCode RegisterService(const std::string& service_name, ServicePtr& service, bool auto_start = true);

  /// @brief Register service by code, config.service_name must be not register by `RegisterService`
  /// @param config service config, config.name must be unique
  /// @param service service object
  /// @param auto_start whether to automatically start and service
  /// @return RegisterRetCode
  RegisterRetCode RegisterService(const ServiceConfig& config, ServicePtr& service, bool auto_start = true);

  /// @brief Dynamically start the service
  /// @param service_name service names
  /// @return true: success; false: failed
  /// @note  service must be registered first
  bool StartService(const std::string& service_name);

  /// @brief Dynamically start the service
  /// @param config Service config
  /// @param service The service to start
  /// @return true: success; false: failed
  bool StartService(const ServiceConfig& config, ServicePtr& service);

  /// @brief Dynamically start the service
  /// @param service_name service name
  /// @param clean_conn Whether to clear the established connection flag,
  ///        true : clear the established connection
  ///        false: stop listening for connections
  /// @return true: success; false: failed
  bool StopService(const std::string& service_name, bool clean_conn);

  /// @brief Get the state of server executation
  ServerState GetServerState() const { return state_; }

  /// @brief Get internal admin service
  AdminServicePtr GetAdminService();

  /// @brief Set/Get whether to enable the ability of name-service heartbeat
  void SetServerHeartBeatReportSwitch(bool report_switch);

  /// @brief Get whether to enable the ability of name-service heartbeat
  bool GetServerHeartBeatReportSwitch();

 private:
  void Stop();
  void BuildAdminServiceAdapter();
  void BuildBusinessServerAdapter();
  ServiceAdapterPtr BuildServiceAdapter(const ServiceConfig& config);
  void BuildServiceAdapterOption(const ServiceConfig& config, ServiceAdapterOption& option);
  int RegisterName(const ServiceAdapterPtr& service_adapter);
  int UnregisterName(const ServiceAdapterPtr& service_adapter);
  void BuildTrpcRegistryInfo(const ServiceAdapterOption& option, TrpcRegistryInfo& registry_info);
  bool CheckSharedTransportConfig(const ServiceConfig& first_service_conf, const ServiceConfig& service_conf);
  std::string GetSharedKey(const ServiceConfig& config);
  void RegisterServiceHeartBeatInfo(const std::string& service_name, const ServiceAdapterPtr& service_adapter);

 private:
  // stop running
  bool terminate_;

  ServerState state_{ServerState::kUnInitialize};

  // the function to stop the server
  ServerTerminateFunction terminate_function_;

  // server config
  ServerConfig server_config_;

  // collection of services
  // key: service name, name must be unique
  // value: service object
  std::unordered_map<std::string, ServiceAdapterPtr> service_adapters_;

  // for the scenario where multiple services share the same transport
  // key: combined information, include: ip,port,network,protocol
  // value: service adapter
  std::unordered_map<std::string, ServiceAdapterPtr> service_shared_adapters_;

  // admin service
  std::string admin_service_name_;
  AdminServicePtr admin_service_;
};

/// @brief Get the global TrpcServer
/// @note  it should be used after the framework runtime environment
///        and plugins has been initialized
std::shared_ptr<TrpcServer> GetTrpcServer();

}  // namespace trpc
