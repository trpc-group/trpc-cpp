//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/detail/writing_datagram_list.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <deque>
#include <tuple>

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief A writing datagram list using with lock which is thread-safe
class WritingDatagramList {
 public:
  /// @brief Send udp packet
  /// @param socket the socket to send data
  /// @param conn_handler connection handler
  /// @param emptied whether the data has all been sent
  /// @return ssize_t the size of the data that has been sent
  ssize_t FlushTo(Socket& socket, ConnectionHandler* conn_handler, bool* emptied);

  /// @brief Append the udp packet to be sent to the tail of the list
  bool Append(NetworkAddress to, IoMessage&& io_msg);

 private:
  mutable std::mutex lock_;
  std::deque<std::tuple<NetworkAddress, IoMessage>> list_;
};

}  // namespace trpc
