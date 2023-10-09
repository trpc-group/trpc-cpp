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
#include <vector>

#include "trpc/future/future.h"
#include "trpc/naming/common/common_defs.h"

namespace trpc::naming {

/// @brief Service instance registration interface
/// @param info The information struct of the service instance
/// @return int Returns 0 on success, -1 on failure
int Register(const TrpcRegistryInfo& info);

/// @brief Service instance unregistration interface
/// @param info The information struct of the service instance
/// @return int Returns 0 on success, -1 on failure
int Unregister(const TrpcRegistryInfo& info);

/// @brief Synchronous interface for reporting service heartbeats
/// @param info The information struct of the service instance
/// @return int Returns 0 on success, -1 on failure
int HeartBeat(const TrpcRegistryInfo& info);

/// @brief Asynchronous interface for reporting service heartbeats
/// @param info The information struct of the service instance
/// @return Future<> Returns a Future<> object that can be used to check
/// if the heartbeat was successfully reported
/// (IsReady() returns true) or if there was an error
/// (IsReady() returns false and the error message can be obtained with GetException())
Future<> AsyncHeartBeat(const TrpcRegistryInfo& info);

/// @brief Synchronously selects a routing node for a service.
/// @param[in] info Information about the service being selected.
/// @param[out] endpoint The selected endpoint.
/// @return int Returns 0 on success, -1 on failure.
int Select(const TrpcSelectorInfo& info, TrpcEndpointInfo& endpoint);

/// @brief Asynchronously selects a routing node for a service.
/// @param info Information about the service being selected.
/// @return Future<TrpcEndpointInfo> Returns a Future object that can be used to
/// check if the selection was successful (IsReady() returns true) or if there was
/// an error (IsReady() returns false and the error message can be obtained with GetException()).
Future<TrpcEndpointInfo> AsyncSelect(const TrpcSelectorInfo& info);

/// @brief Synchronously selects multiple routing nodes for a service according to a given strategy.
/// @param[in] info Information about the service being selected.
/// @param[out] endpoints The selected endpoints.
/// @return int Returns 0 on success, -1 on failure.
int SelectBatch(const TrpcSelectorInfo& info, std::vector<TrpcEndpointInfo>& endpoints);

/// @brief Asynchronously selects multiple routing nodes for a service according to a given strategy.
/// @param info Information about the service being selected.
/// @return Future<std::vector<TrpcEndpointInfo>> Returns a Future object that
/// can be used to check if the selection was successful (IsReady() returns true) or
/// if there was an error (IsReady() returns false and the error message can be obtained with GetException()).
Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const TrpcSelectorInfo& info);

/// @brief Reports the result of a service discovery call.
/// @param result Information about the service discovery call result.
/// @return int Returns 0 on success, -1 on failure.
int ReportInvokeResult(const TrpcInvokeResult& result);

/// @brief Sets the routing information for a service.
/// @param info The routing information.
/// @return int Returns 0 on success, -1 on failure.
int SetEndpoints(const TrpcRouterInfo& info);

/// @brief Sets the whitelist of framework error codes for circuit breaking reporting.
/// @param white_list The whitelist of framework error codes.
/// @return int Returns true on success, false on failure.
/// @note Not thread-safe, please set after plugin initialization and before RPC call initiation, and call only once.
bool SetCircuitBreakWhiteList(const TrpcCircuitBreakWhiteList& white_list);

/// @brief Gets the rate limiting status to determine if rate limiting is triggered.
/// @param info Information about the rate limiting for a service.
/// @return LimitRetCode Returns kLimitOK if not rate limited, kLimitReject if
/// rate limited and request is rejected, kLimitError if an error occurs during rate limiting.
LimitRetCode ShouldLimit(const TrpcLimitInfo& info);

/// @brief Finishes rate limiting processing, such as reporting some rate limiting data.
/// @param result Information about the rate limiting call result.
/// @return int Returns 0 on success, -1 on failure.
int FinishLimit(const TrpcLimitResult& result);

}  // namespace trpc::naming
