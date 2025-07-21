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

#include <type_traits>
#include <vector>

#include "trpc/codec/trpc/trpc.pb.h"

namespace trpc::codec {

/// @brief Return codes uniformly defined by different protocols on the server side
enum class ServerRetCode : int {
  SUCCESS = 0,                  ///< Invoke successfully.
  TIMEOUT_ERROR = 1,            ///< Request timed out on the server side.
  OVERLOAD_ERROR = 2,           ///< The request was discarded on the server side due to overload protection.
  LIMITED_ERROR = 3,            ///< The request was rate-limited on the server side.
  FULL_LINK_TIMEOUT_ERROR = 4,  ///< The request timed out on the server side due to the full-link duration timeout.
  ENCODE_ERROR = 5,             ///< Server-side encoding error.
  DECODE_ERROR = 6,             ///< Server-side decoding error.
  NOT_FUN_ERROR = 7,            ///< The server did not invoke the corresponding interface implementation.
  AUTH_ERROR = 8,               ///< Authentication/authorization error on the server side.
  INVOKE_UNKNOW_ERROR = 9,      ///< Unknown error.
};

/// @brief Return codes uniformly defined by different protocols on the client side
enum class ClientRetCode : int {
  SUCCESS = 0,                  ///< Invoke successfully.
  INVOKE_TIMEOUT_ERR = 1,       ///< Request timed out on the client side.
  FULL_LINK_TIMEOUT_ERROR = 2,  ///< Request timed out on the client side due to the full chain timeout.
  CONNECT_ERROR = 3,            ///< Client connection error.
  ENCODE_ERROR = 4,             ///< Client encoding error.
  DECODE_ERROR = 5,             ///< Client decoding error.
  LIMITED_ERROR = 6,            ///< Request was rate-limited on the client side.
  OVERLOAD_ERROR = 7,           ///< Request was discarded on the client side due to overload protection.
  ROUTER_ERROR = 8,             ///< Client IP routing error.
  NETWORK_ERROR = 9,            ///< Client network error.
  INVOKE_UNKNOW_ERROR = 10,     ///< Unknown error.
};

/// @brief The default return code of the server, if `ServerCodec` does not implement `GetProtocolRetCode`
const std::vector<int> kDefaultServerRetcodes{TrpcRetCode::TRPC_INVOKE_SUCCESS,
                                              TrpcRetCode::TRPC_SERVER_TIMEOUT_ERR,
                                              TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR,
                                              TrpcRetCode::TRPC_SERVER_LIMITED_ERR,
                                              TrpcRetCode::TRPC_SERVER_FULL_LINK_TIMEOUT_ERR,
                                              TrpcRetCode::TRPC_SERVER_ENCODE_ERR,
                                              TrpcRetCode::TRPC_SERVER_DECODE_ERR,
                                              TrpcRetCode::TRPC_SERVER_NOFUNC_ERR,
                                              TrpcRetCode::TRPC_SERVER_AUTH_ERR,
                                              TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR};

/// @brief The default return code of the client, if `ServerCodec` does not implement `GetProtocolRetCode`
const std::vector<int> kDefaultClientRetcodes{TrpcRetCode::TRPC_INVOKE_SUCCESS,
                                              TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR,
                                              TrpcRetCode::TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR,
                                              TrpcRetCode::TRPC_CLIENT_CONNECT_ERR,
                                              TrpcRetCode::TRPC_CLIENT_ENCODE_ERR,
                                              TrpcRetCode::TRPC_CLIENT_DECODE_ERR,
                                              TrpcRetCode::TRPC_CLIENT_LIMITED_ERR,
                                              TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR,
                                              TrpcRetCode::TRPC_CLIENT_ROUTER_ERR,
                                              TrpcRetCode::TRPC_CLIENT_NETWORK_ERR,
                                              TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR};

template <typename E>
constexpr auto ToUType(E enumerator) noexcept {
  return static_cast<std::underlying_type_t<E>>(enumerator);
}

/// @brief Get default return code by ServerRetCode
int GetDefaultServerRetCode(ServerRetCode ret_code);

/// @brief Get default return code by ClientRetCode
int GetDefaultClientRetCode(ClientRetCode ret_code);

}  // namespace trpc::codec
