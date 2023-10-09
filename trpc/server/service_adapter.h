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
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "trpc/codec/server_codec.h"
#include "trpc/server/service.h"
#include "trpc/server/service_adapter_option.h"

namespace trpc {

class ThreadModel;

/// @brief An adaptation class that connects the lower layer transport and
///        the upper layer service processing
class ServiceAdapter {
 public:
  explicit ServiceAdapter(ServiceAdapterOption&& option);

  /// @brief Start listening for connections
  bool Listen();

  /// @brief Start listening for connections
  /// @param clean_conn true: indicates clearing established connections
  bool StopListen(bool clean_conn);

  /// @brief Stop listen and close connection
  void Stop();

  /// @brief Destroy resource
  void Destroy();

  /// @brief Set the service object for business registration
  void SetService(const ServicePtr& service);

  /// @brief Get service name
  const std::string& GetServiceName() const { return option_.service_name; }

  /// @brief Get server adpater option
  /// @return ServiceAdapterOption
  const ServiceAdapterOption& GetServiceAdapterOption() const { return option_; }

  /// @brief Get protocol name of service
  /// @return std::string
  const std::string& GetProtocol() const { return option_.protocol; }

  /// @brief Get server codec
  /// @return ServerCodecPtr
  /// @note It is not `const` ServerCodecPtr
  ServerCodecPtr& GetServerCodec() { return server_codec_; }

  /// @brief Get server codec
  /// @return ServerCodecPtr
  /// @note It is `const` ServerCodecPtr
  const ServerCodecPtr& GetServerCodec() const { return server_codec_; }

  /// @brief the processing method of the request message received by transport
  /// @param conn connection object
  /// @param msg list of messages received by the connection
  /// @return true: handle success
  ///         false: handle failed
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg);

  /// @brief the processing method of the request message received by transport on fiber threadmodel
  /// @param conn connection object
  /// @param msg list of messages received by the connection
  /// @return true: handle success
  ///         false: handle failed
  bool HandleFiberMessage(const ConnectionPtr& conn, std::deque<std::any>& msg);

  /// @brief set automatic listening to true
  ///        when TRpcServer starts, it will start listening services
  void SetAutoStart() { auto_start_ = true; }

  /// @brief get automatic listening flag
  bool IsAutoStart() const { return auto_start_; }

  /// @brief set whether multiple services share a transport
  void SetIsShared(bool is_shared) { is_shared_ = is_shared; }

  /// @brief get whether multiple services share a transport
  bool GetIsShared() const { return is_shared_; }

 private:
  bool Initialize();
  std::unique_ptr<ServerTransport> CreateServerTransport();
  void FillBindInfo(trpc::BindInfo& bind_info);
  void SetSSLConfigToBindInfo(trpc::BindInfo& bind_info);
  STransportReqMsg* CreateSTransportReqMsg(const ConnectionPtr& conn, uint64_t recv_timestamp_us,
                                           std::any&& msg);
  Service* ChooseService(Protocol* protocol) const;
  void AddServiceFilter(ServerFilterController& filter_controller, const MessageServerFilterPtr& filter);

 private:
  // option
  ServiceAdapterOption option_;

  // transport
  std::unique_ptr<ServerTransport> transport_{nullptr};

  // the threading model used by the service, not owned
  ThreadModel* thread_model_{nullptr};

  // the service object of business register
  ServicePtr service_{nullptr};

  // codec
  ServerCodecPtr server_codec_{nullptr};

  // multiple services that shared a transport
  std::unordered_map<std::string, ServicePtr> services_;

  // whether multiple services share a transport
  bool is_shared_{false};

  // whether automatic listening when trpc server start
  bool auto_start_{false};

  // whether listening flag
  bool is_listened_{false};
};

using ServiceAdapterPtr = std::shared_ptr<ServiceAdapter>;

}  // namespace trpc
