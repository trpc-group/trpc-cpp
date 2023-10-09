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

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "trpc/codec/server_codec.h"
#include "trpc/filter/server_filter_controller.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/default/acceptor.h"
#include "trpc/server/non_rpc/non_rpc_service_method.h"
#include "trpc/server/rpc/rpc_service_method.h"
#include "trpc/server/service_adapter_option.h"
#include "trpc/transport/server/server_transport.h"

namespace trpc {

class ServiceAdapter;
class ThreadModel;

/// @brief The function for dispatching request to specified handle-thread
///        generally used in merge and separate threading models
using HandleRequestDispatcherFunction = std::function<int32_t(const STransportReqMsg* req)>;

/// @brief The function for user-defined processing request after request timeout
using ServiceTimeoutHandleFunction = std::function<void(const ServerContextPtr& context)>;

/// @brief The function for user self-defined set socket option
using CustomSocketOptFunction = SetSocketOptFunction;

/// @brief Base class for service
class Service {
 public:
  virtual ~Service();

  /// @brief process unary request message received by the transport and return the response
  /// @param[in] recv  transport request message, not owned,
  ///                  the caller of this method created it by object pool,
  ///                  the caller of this method needs to return it to the object pool.
  /// @param[out] send transport response message, if *send not nullptr, it should created by object pool,
  ///                  the caller of this method needs to return it to the object pool.
  virtual void HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept = 0;

  /// @brief process stream request message received by the transport
  virtual void HandleTransportStreamMessage(STransportReqMsg* recv) noexcept {}

  /// @brief check if the methods in Service contains the streaming rpc method
  /// @return true: exist, false: not exist
  virtual bool AnyOfStreamingRpc() {
    return std::any_of(rpc_service_methods_.begin(), rpc_service_methods_.end(),
                       [](const std::pair<std::string, RpcServiceMethod*>& method) {
                         return method.second->GetMethodType() != MethodType::UNARY;
                       });
  }

  /// @brief fill fileds of server context.
  virtual void FillServerContext(const ServerContextPtr& context) {}

  /// @brief set service name.
  void SetName(const std::string& name) { name_ = name; }

  /// @brief get service name.
  const std::string& GetName() const { return name_; }

  /// @brief Framework use or for testing. Set service transport.
  void SetServerTransport(ServerTransport* transport) { transport_ = transport; }

  /// @brief Framework use or for testing. Get service transport.
  ServerTransport* GetServerTransport() { return transport_; }

  /// @brief Create transport.
  virtual std::unique_ptr<ServerTransport> CreateTransport(const ServiceAdapterOption& options,
                                                           ThreadModel* thread_model) {
    return nullptr;
  }

  /// @brief Framework use or for testing. Set the `ServiceAdapter` pointer to which this service belongs.
  void SetAdapter(ServiceAdapter* adapter) { adapter_ = adapter; }

  /// @brief Framework use or for testing. Get the `ServiceAdapter` pointer to which this service belongs.
  ServiceAdapter* GetAdapter() { return adapter_; }

  /// @brief Get filter controller.
  ServerFilterController& GetFilterController() { return filter_controller_; }

  /// @brief Get server adpater option.
  const ServiceAdapterOption& GetServiceAdapterOption() const;

  /// @brief Framework use or for testing. Set wether to use FiberExecutionContext.
  void SetNeedFiberExecutionContext(bool need_fiber_ctx) { need_fiber_ctx_ = need_fiber_ctx; }

  /// @brief Framework use or for testing. Get wether to use FiberExecutionContext.
  bool GetNeedFiberExecutionContext() const { return need_fiber_ctx_; }

  /// @brief Add rpc service method.
  void AddRpcServiceMethod(RpcServiceMethod* method) { rpc_service_methods_[method->Name()] = method; }

  /// @brief Get rpc service method.
  const std::unordered_map<std::string, RpcServiceMethod*>& GetRpcServiceMethod() const { return rpc_service_methods_; }

  /// @brief Add non-rpc service method.
  void AddNonRpcServiceMethod(NonRpcServiceMethod* method) { non_rpc_service_methods_[method->Name()] = method; }

  /// @brief Get non-rpc service method.
  const std::unordered_map<std::string, NonRpcServiceMethod*>& GetNonRpcServiceMethod() const {
    return non_rpc_service_methods_;
  }

  /// @brief Set the function for dispatching request to specified handle-thread.
  void SetHandleRequestDispatcherFunction(const HandleRequestDispatcherFunction& function) {
    dispatcher_func_ = function;
  }

  /// @brief Get the function for dispatching request to specified handle-thread.
  HandleRequestDispatcherFunction& GetHandleRequestDispatcherFunction() { return dispatcher_func_; }

  /// @brief Set the function for user-defined processing request after request timeout.
  void SetServiceTimeoutHandleFunction(const ServiceTimeoutHandleFunction& timeout_function) {
    service_timeout_handle_function_ = timeout_function;
  }

  /// @brief Get the function for user-defined processing request after request timeout.
  ServiceTimeoutHandleFunction& GetServiceTimeoutHandleFunction() { return service_timeout_handle_function_; }

  /// @brief Set the user's custom processing function after the server receives the connection.
  void SetAcceptConnectionFunction(const AcceptConnectionFunction& accept_connection_function) {
    accept_connection_function_ = accept_connection_function;
  }

  /// @brief Get the user's custom processing function after the server receives the connection
  AcceptConnectionFunction GetAcceptConnectionFunction() { return accept_connection_function_; }

  /// @brief Set the function for user self-defined set socket option
  void SetCustomSetSocketOptFunction(const CustomSocketOptFunction& custom_set_socket_opt_function) {
    custom_set_socket_opt_function_ = custom_set_socket_opt_function;
  }

  /// @brief Get the function for user self-defined set socket option
  CustomSocketOptFunction GetCustomSetSocketOptFunction() { return custom_set_socket_opt_function_; }

  /// @brief Set the function for user self-defined set listening socket option
  void SetCustomSetAcceptSocketOptFunction(const CustomSocketOptFunction& custom_set_socket_opt_function) {
    custom_set_accept_socket_opt_function_ = custom_set_socket_opt_function;
  }

  /// @brief Get the function for user self-defined set listening socket option
  CustomSocketOptFunction GetCustomSetAcceptSocketOptFunction() { return custom_set_accept_socket_opt_function_; }

  /// @brief Set the function for dispatching network connection to specific thread
  void SetDispatchAcceptConnectionFunction(const DispatchAcceptConnectionFunction& dispatch_accept_conn_function) {
    dispatch_accept_conn_function_ = dispatch_accept_conn_function;
  }

  /// @brief Set the function for dispatching network connection to specific thread
  DispatchAcceptConnectionFunction GetDispatchAcceptConnectionFunction() { return dispatch_accept_conn_function_; }

  /// @brief Set the user callback processing function after the connection is established
  void SetConnectionEstablishFunction(const ConnectionEstablishFunction& connection_establish_function) {
    connection_establish_function_ = connection_establish_function;
  }

  /// @brief Get the user callback processing function after the connection is established
  ConnectionEstablishFunction GetConnectionEstablishFunction() { return connection_establish_function_; }

  /// @brief Set the user callback processing function after the connection is closed
  void SetConnectionCloseFunction(const ConnectionCloseFunction& connection_close_function) {
    connection_close_function_ = connection_close_function;
  }

  /// @brief Get the user callback processing function after the connection is closed
  ConnectionCloseFunction GetConnectionCloseFunction() { return connection_close_function_; }

  /// @brief Set a custom protocol check function
  /// @note The inspection and encoding and decoding of custom protocol are generally implemented
  ///       through the codec plug-in, and this method is only required for special needs
  void SetProtocalCheckerFunction(const ProtocolCheckerFunction& protocol_checker_function) {
    protocol_checker_function_ = protocol_checker_function;
  }

  /// @brief Set a custom protocol check function
  ProtocolCheckerFunction GetProtocalCheckerFunction() { return protocol_checker_function_; }

  /// @brief Set the handler function of custom message
  /// @note  This method is only required for special needs with CheckerFunction
  void SetMessageHandleFunction(const MessageHandleFunction& message_handle_function) {
    message_handle_function_ = message_handle_function;
  }

  /// @brief Get the handler function of custom message
  MessageHandleFunction GetMessageHandleFunction() { return message_handle_function_; }

  /// @brief Send response.
  int SendMsg(STransportRspMsg* msg);

  /// @brief Reports whether the connection identified by |conn_id} is connected with peer.
  bool IsConnected(uint64_t conn_id);

  /// @brief Closes the connection identified by |conn|.
  void CloseConnection(const CloseConnectionInfo& conn);

  /// @brief Connection throttling is a technique used to control the flow of data over a connection by
  ///        pausing the reading of data from the connection
  void ThrottleConnection(uint64_t conn_id, bool set);

 protected:
  // service name
  std::string name_;

  // transport, not owned
  ServerTransport* transport_{nullptr};

  // The `ServiceAdapter` object to which the service belongs is mainly used for scenarios
  // where asynchronous response is required for requests.
  ServiceAdapter* adapter_{nullptr};

  // rpc service methods
  std::unordered_map<std::string, RpcServiceMethod*> rpc_service_methods_;

  // non-rpc service methods
  std::unordered_map<std::string, NonRpcServiceMethod*> non_rpc_service_methods_;

  // filter
  ServerFilterController filter_controller_;

  // whether to create fiber context
  bool need_fiber_ctx_{false};

  // the function for dispatching request to specified handle-thread
  // merge/separate threadmodel use
  HandleRequestDispatcherFunction dispatcher_func_{nullptr};

  // the function for user-defined processing request after request timeout
  ServiceTimeoutHandleFunction service_timeout_handle_function_{nullptr};

  // the user's custom processing function after the server receives the connection
  AcceptConnectionFunction accept_connection_function_{nullptr};

  // the function for user self-defined set socket option
  CustomSocketOptFunction custom_set_socket_opt_function_{nullptr};

  // the function for user self-defined set listening socket option
  CustomSocketOptFunction custom_set_accept_socket_opt_function_{nullptr};

  // the function for dispatching network connection to specific thread
  // generally merge/separate threadmodel use
  DispatchAcceptConnectionFunction dispatch_accept_conn_function_{nullptr};

  // the user callback processing function after the connection is established
  ConnectionEstablishFunction connection_establish_function_{nullptr};

  // the user callback processing function after the connection is closed
  ConnectionCloseFunction connection_close_function_{nullptr};

  // a custom protocol check function
  ProtocolCheckerFunction protocol_checker_function_{nullptr};

  // the handler function of custom message
  MessageHandleFunction message_handle_function_{nullptr};
};

using ServicePtr = std::shared_ptr<Service>;

}  // namespace trpc
