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

#include <functional>
#include <string>

#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/common/unix_address.h"

namespace trpc {

/// @brief The information of the connection
struct ConnectionInfo {
  // true: net, false: unix.
  bool is_net = true;

  NetworkAddress local_addr;
  NetworkAddress remote_addr;
  UnixAddress un_local_addr;
  UnixAddress un_remote_addr;

  std::string ToString() const {
    std::string str = "[Foreign] ";
    str += is_net ? remote_addr.ToString() : un_remote_addr.Path();
    str += " [Local] ";
    str += is_net ? local_addr.ToString() : un_local_addr.Path();
    return str;
  }
};

/// @brief the information of the connection received by the server,
///        including local/remote ip/port and socket
struct AcceptConnectionInfo {
  Socket socket;
  ConnectionInfo conn_info;
};

/// @brief // the function of proccessing connection after the server receives
using AcceptConnectionFunction = std::function<bool(AcceptConnectionInfo& info)>;

/// @brief the function for dispatching network connection to specific thread
///        generally merge/separate threadmodel use
using DispatchAcceptConnectionFunction = std::function<std::size_t(const AcceptConnectionInfo&, std::size_t)>;

}  // namespace trpc
