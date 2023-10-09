
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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <string>
#include <string_view>

#include "trpc/client/client_context.h"
#include "trpc/rpcz/span.h"
#include "trpc/server/server_context.h"
#include "trpc/util/http/request.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

/// @brief Namespace to wrap rpcz interfaces.
namespace trpc::rpcz {

/// @brief Create a new server span object in first filter point and fill basic info.
/// @param context Server context.
/// @param req_seq_id Unique id for this span.
/// @param recv_timestamp_us Timestamp recv this request.
/// @return Span ptr.
/// @private
Span* CreateServerRpczSpan(const trpc::ServerContextPtr& context, uint32_t req_seq_id,
                                                    const uint64_t recv_timestamp_us);

/// @brief Create a new client span object in first filter point and fill basic info.
/// @param context Client context.
/// @param req_seq_id Unique id for this span.
/// @return Span ptr.
/// @private
Span* CreateClientRpczSpan(const trpc::ClientContextPtr& context, uint32_t req_seq_id);

/// @brief For the kSpanTypeUser type of span object, users are provided with the ability to
///        create a root span object without context.
/// @param viewer_name The name of the root span object, used for external display.
/// @return If the creation fails, nullptr is returned. If successful, a pointer to the root
///         span object is returned.
/// @note Since there is no place to store the root span object, users need to pass the root
///       span object themselves during usage. Users can manipulate the root span object but
///       should not actively release it. The framework is responsible for releasing the root
///       span object uniformly.
Span* CreateUserRpczSpan(const std::string& viewer_name);

/// @brief For the kSpanTypeUser type of span object, users are provided with the ability to
///        create a root span object with context.
/// @param viewer_name The name of the root span object, used for external display.
/// @param context Context used to store the pointer of the root span object.
/// @return If the creation fails, nullptr is returned. If successful, a pointer to the root
///         span object is returned.
/// @note Users can obtain the root span object through the GetUserRpczSpan interface.
///       Users can manipulate the root span object but should not actively release it.
///       The framework is responsible for releasing the root span object uniformly.
Span* CreateUserRpczSpan(const std::string& viewer_name, trpc::ServerContextPtr& context);

/// @brief For span objects of type kSpanTypeUser, extract the root span object from the context.
/// @param context Context used to store the pointer of the root span object.
/// @return If not exist, nullptr is returned; otherwise, a pointer to the root span object is returned.
Span* GetUserRpczSpan(const trpc::ServerContextPtr& context);

/// @brief For span objects of type kSpanTypeUser, submit the root span object to the framework.
/// @note The interface provided to users can only be called on the root span object. After calling
///       this interface, users cannot manipulate the root span object anymore.
///       Ideally, this interface should be designed as a member function of the Span class. However,
///       to avoid a circular dependency between Span and RpczCollector in Bazel, it is implemented
///       differently.
void SubmitUserRpczSpan(Span* root_span_ptr);

/// @brief Entry for admin query.
/// @param req Http request.
/// @return Result span info.
/// @private
std::string TryGet(const trpc::http::HttpRequestPtr& req);

/// @brief Let user print custom log in callback.
/// @param context Server context.
/// @param filename_in Which file to invoke current function.
/// @param line_in Which line to invoke current function.
/// @param msg Custom log by user.
/// @private
void RpczPrint(const trpc::ServerContextPtr& context,
               const char* filename_in,
               int line_in,
               const std::string_view& msg);

}  // namespace trpc::rpcz
#endif
