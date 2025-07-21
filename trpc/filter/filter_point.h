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

namespace trpc {

/// The prefix for server filter points, it allows for a maximum of 128 server filter points.
constexpr uint8_t kServerFilterPrefix = 0x80;

/// The mask for server filter points.
constexpr uint8_t kServerFilterMask = 0x7f;

/// @brief Enumeration of filter points : including pre and post filter points (there are paired).
///        The index of the paired filter points is (index, index+1), where index is an even number.
/// @note The number of filter points between the client and server must be consistent.
enum class FilterPoint {
  CLIENT_PRE_RPC_INVOKE = 0,   ///< Before makes client RPC call
  CLIENT_POST_RPC_INVOKE = 1,  ///< After makes client RPC call

  CLIENT_PRE_SEND_MSG = 2,   ///< Before client sends the RPC request message
  CLIENT_POST_RECV_MSG = 3,  ///< After client receives the RPC response message

  CLIENT_PRE_SCHED_SEND_MSG = 4,   ///< Before schedules client's sending request for processing
  CLIENT_POST_SCHED_SEND_MSG = 5,  ///< After schedules client's sending request for processing

  CLIENT_PRE_SCHED_RECV_MSG = 6,   ///< Before schedules client's recevied response for processing
  CLIENT_POST_SCHED_RECV_MSG = 7,  ///< After schedules client's recevied response for processing

  CLIENT_PRE_IO_SEND_MSG = 8,   ///< Before performs the I/O operation for sending request
  CLIENT_POST_IO_SEND_MSG = 9,  ///< After performs the I/O operation for sending request

  SERVER_POST_RECV_MSG = kServerFilterPrefix | 0,  ///< After server receives the RPC request message
  SERVER_PRE_SEND_MSG = kServerFilterPrefix | 1,   ///< Before server sends the RPC response message

  SERVER_PRE_RPC_INVOKE = kServerFilterPrefix | 2,   ///< Before makes server RPC call
  SERVER_POST_RPC_INVOKE = kServerFilterPrefix | 3,  ///< After makes server RPC call

  SERVER_PRE_SCHED_RECV_MSG = kServerFilterPrefix | 4,   ///< Before schedules server's received request for processing
  SERVER_POST_SCHED_RECV_MSG = kServerFilterPrefix | 5,  ///< After schedules server's received request for processing

  SERVER_PRE_SCHED_SEND_MSG = kServerFilterPrefix | 6,   ///< Before schedules server's sending response for processing
  SERVER_POST_SCHED_SEND_MSG = kServerFilterPrefix | 7,  ///< After schedules server's sending response for processing

  SERVER_PRE_IO_SEND_MSG = kServerFilterPrefix | 8,   ///< Before performs the I/O operation for sending response
  SERVER_POST_IO_SEND_MSG = kServerFilterPrefix | 9,  ///< After performs the I/O operation for sending response
};

// Total number of filter points.
constexpr int kFilterNum = 20;

// The number of filter points for the client and server respectively.
constexpr int kFilterTypeNum = kFilterNum / 2;

/// @brief Get the paired filter point.
/// @param point filter point
/// @return point the paired filter point
inline FilterPoint GetMatchPoint(FilterPoint point) {
  auto index = static_cast<int>(point);
  if ((index & 0x01) == 0) {  // index is even
    return static_cast<FilterPoint>(index + 1);
  }

  return static_cast<FilterPoint>(index - 1);
}

}  // namespace trpc
