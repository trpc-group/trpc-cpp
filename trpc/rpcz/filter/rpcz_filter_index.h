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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <any>

#include "trpc/rpcz/span.h"
#include "trpc/util/unique_id.h"

namespace trpc::rpcz {

// For pure server rpcz.
/// @private
const uint32_t kTrpcServerRpczIndex = 14;
// For route (server + client) rpcz.
/// @private
const uint32_t kTrpcRouteRpczIndex = 15;
// For pure client rpcz.
/// @private
const uint32_t kTrpcClientRpczIndex = 16;
// For user-defined rpcz.
/// @private
const uint32_t kTrpcUserRpczIndex = 17;

/// @brief To generate unique span id.
/// @private
class SpanIdGenerator {
 public:
  /// @brief Default constructor.
  SpanIdGenerator() = default;

  /// @brief Get global instance.
  static SpanIdGenerator* GetInstance() {
    static SpanIdGenerator instance;
    return &instance;
  }

  /// @brief Disable copy and assignment.
  SpanIdGenerator(const SpanIdGenerator&) = delete;
  SpanIdGenerator& operator=(const SpanIdGenerator&) = delete;

  /// @brief Generate id.
  uint32_t GenerateId() {
    uint32_t id = rpcz_span_id_generator_.GenerateId();
    return id;
  }

 private:
  trpc::UniqueId rpcz_span_id_generator_;
};

/// @brief To be stored in server context.
/// @private
struct ServerRpczSpan {
  bool sample_flag{false};
  Span* span;
};

/// @brief For route scenario.
/// @private
struct ClientRouteRpczSpan {
  bool sample_flag{false};
  // Normally server span.
  Span* parent_span;
  Span* span{nullptr};
};

/// @brief To be stored in client context.
/// @private
struct ClientRpczSpan {
  bool sample_flag{false};
  Span* span;
};

/// @brief In user-defined rpcz scenario, this is the data type that exists within the context.
///        To facilitate future expansion, a separate structure is created.
/// @private
struct UserRpczSpan {
  Span* span;
};

}  // namespace trpc::rpcz
#endif
