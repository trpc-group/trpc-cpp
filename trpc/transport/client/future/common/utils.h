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

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"
#include "trpc/runtime/iomodel/reactor/default/udp_transceiver.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/future/common/future_connector_options.h"
#include "trpc/transport/client/future/common/future_connection_handler.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::future {

/// @brief Dispatch normal response.
/// @param req_msg Request message, current function do not own it.
/// @param rsp_msg Response message, must be created from object pool, will
///                be returned to object popl in this function.
/// @param rsp_dispatch_function Function to decide which handle thread to dispatch to.
/// @param run_client_filters_function Function to run client filters.
void DispatchResponse(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg,
                      const TransInfo::RspDispatchFunction& rsp_dispatch_function,
                      const TransInfo::RunClientFiltersFunction& run_client_filters_function = nullptr);

/// @brief Dispatch exception when request/response error.
/// @param req_msg Request message, current function do not own it.
/// @param ret Error code.
/// @param err_msg Error message.
/// @param rsp_dispatch_function Function to decide which handle thread to dispatch to.
/// @param run_client_filters_function Function to run client filters.
void DispatchException(CTransportReqMsg* req_msg, int ret, std::string&& err_msg,
                       const TransInfo::RspDispatchFunction& rsp_dispatch_function,
                       const TransInfo::RunClientFiltersFunction& run_client_filters_function = nullptr);

/// @brief To trigger backup request withdraw.
/// @param promise Related Promise of backup request.
/// @note Type of promise must be Promise<bool>.
void NotifyBackupRequestOver(void*& promise);

/// @brief To trigger backup request resend.
/// @param ex Exception.
/// @param promise Related Promise of backup request.
/// @note Type of promise must be Promise<bool>.
void NotifyBackupRequestResend(Exception&& ex, void*& promise);

/// @brief Get connection state.
/// @param conn Target connection.
/// @return State of connection.
inline ConnectionState GetConnectionState(Connection* conn) {
  if (TRPC_UNLIKELY(conn == nullptr)) {
    return ConnectionState::kUnconnected;
  }
  return conn->GetConnectionState();
}

/// @brief Get connection state, with unique_ptr parameter.
/// @param conn Target connection.
/// @return State of connection.
inline ConnectionState GetConnectionState(const std::unique_ptr<Connection>& conn) {
  if (TRPC_UNLIKELY(conn == nullptr)) {
    return ConnectionState::kUnconnected;
  }
  return conn->GetConnectionState();
}

/// @brief Send tcp message.
/// @param req_msg Request message.
/// @param conn Target connection.
/// @param is_oneway Send only or not.
/// @note conn must not be nullptr.
int SendTcpMsg(CTransportReqMsg* req_msg, Connection* conn, bool is_oneway = false);

/// @brief Send udp message.
/// @param req_msg Request message.
/// @param conn Target connection.
/// @note conn must not be nullptr.
int SendUdpMsg(CTransportReqMsg* req_msg, Connection* conn);

/// @brief Create a tcp connection.
/// @param conn_options Options of connector.
/// @param conn_handler Connection handler.
RefPtr<TcpConnection> CreateTcpConnection(const FutureConnectorOptions& conn_options,
                                          std::unique_ptr<FutureConnectionHandler>&& conn_handler);

/// @brief Create a udp transreceiver.
/// @param conn_options Options of connector.
/// @param conn_handler Connection handler.
RefPtr<UdpTransceiver> CreateUdpTransceiver(const FutureConnectorOptions& conn_options,
                                            std::unique_ptr<FutureConnectionHandler>&& conn_handler);

}  // namespace trpc::future
