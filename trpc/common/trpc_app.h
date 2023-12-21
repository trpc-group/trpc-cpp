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

#include <memory>
#include <string>

#include "trpc/admin/admin_handler.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/common/trpc_version.h"
#include "trpc/config/trpc_conf.h"
#include "trpc/server/trpc_server.h"
#include "trpc/util/http/common.h"
#include "trpc/util/log/logging.h"

/// @mainpage tRPC-Cpp API
///

/// @brief Primary namespace of tRPC-Cpp.
namespace trpc {

/// @brief Base class for trpc application program
class TrpcApp {
 public:
  virtual ~TrpcApp() = default;

  /// @brief Framework entry function, including parsing framework configuration files,
  /// framework client/server resource construction, etc.
  /// @return 0: success, -1: failed
  int Main(int argc, char* argv[]);

  /// @brief Wait for the run to finish
  void Wait();

  /// @brief Terminate the program
  void Terminate();

  /// @brief Check whether the terminated state is set
  bool CheckTerminated();

  /// @brief Get framework client
  const std::shared_ptr<TrpcClient>& GetTrpcClient() const { return client_; }

  /// @brief Program terminate signal handler
  /// @param signo usr2
  static void SigUsr2Handler(int signo);

  /// @brief Reload framework configuration file signal handler
  /// @param signo usr1
  static void SigUsr1Handler(int signo);

  /// @brief The interface for initilize the framework configuration by code(not use file)
  /// @return 0: success, other: failure
  virtual int InitFrameworkConfig() { return -1; }

  /// @brief The interface for updating framework configuration content
  virtual void UpdateFrameworkConfig() {}

  /// @brief The interface for registering custom implemented plugins
  /// Override this interface and call `trpc::TrpcPlugin::RegisterXXPlugin(XxPlugin)`
  /// e.g. trpc::TrpcPlugin::RegisterServerCodec(XxServerCodec);
  /// @return 0: success, -1: failed
  virtual int RegisterPlugins() { return 0; }

  /// @brief Business logic initialization
  /// @return 0: success, -1: failed
  virtual int Initialize() = 0;

  /// @brief Business resource destruction
  virtual void Destroy() = 0;

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
  ///        use `StartService` by code
  TrpcServer::RegisterRetCode RegisterService(const std::string& service_name, ServicePtr& service,
                                              bool auto_start = true);
  TrpcServer::RegisterRetCode RegisterService(const std::string& service_name, const ServicePtr& service,
                                              bool auto_start = true);

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
  bool StopService(const std::string& service_name, bool clean_conn = false);

  /// @brief Deprecated: use "RegisterCmd(trpc::http::OperationType type, const std::string& url,
  ///                                     const std::shared_ptr<trpc::AdminHandlerBase>& handler)" instead.
  /// @param type operation type
  /// @param url path
  /// @param handler admin handler
  /// @note User need to manually manage the handler object, handler is not freed by framework
  [[deprecated("use shared_ptr interface for automatic object lifetime management")]]
  void RegisterCmd(trpc::http::OperationType type, const std::string& url, trpc::AdminHandlerBase* handler);

  /// @brief Register custom admin command
  /// @param type operation type
  /// @param url path
  /// @param handler admin handler
  void RegisterCmd(trpc::http::OperationType type, const std::string& url,
                   const std::shared_ptr<trpc::AdminHandlerBase>& handler);

  /// @brief Register configuration update callback function
  /// @param name configuration item
  /// @param cb callback
  void RegisterConfigUpdateNotifier(const std::string& name, const std::function<void(const YAML::Node&)>& cb);

 protected:
  // Parsing framework configuration files
  void ParseFrameworkConfig(int argc, char* argv[]);

  // Run in the background as a daemon
  bool Daemonlize();

  // Initialize the framework runtime environment
  virtual void InitializeRuntime();

  // Execute main logic
  virtual void Run();

  // Destroy the framework runtime environment
  virtual void DestroyRuntime();

  // Execute function
  void Execute();

 protected:
  // server
  std::shared_ptr<TrpcServer> server_;

  // client
  std::shared_ptr<TrpcClient> client_;
};

}  // namespace trpc
