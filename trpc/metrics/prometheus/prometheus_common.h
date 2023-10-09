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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace trpc::prometheus {

/// @brief Prometheus metrics name
constexpr char kPrometheusMetricsName[] = "prometheus";

/// @brief The key definition reported by the filter
constexpr char kCallerAppKey[] = "aApp";
constexpr char kCallerServerKey[] = "aServer";
constexpr char kCallerServiceKey[] = "aService";
constexpr char kCallerIp[] = "aIp";
constexpr char kCalleeAppKey[] = "pApp";
constexpr char kCalleeServerKey[] = "pServer";
constexpr char kCalleeServiceKey[] = "pService";
constexpr char kCalleeFuncName[] = "pInterface";
constexpr char kCalleeIp[] = "pIp";
constexpr char kCalleeContainerName[] = "pContainer";
constexpr char kCalleeSetId[] = "pConSetId";
constexpr char kNamespace[] = "physicEnv";
constexpr char kEnv[] = "userEnv";
constexpr char kFrameRetCode[] = "frame_ret_code";
constexpr char kInterfaceRetCode[] = "interface_ret_code";

/// @private
namespace detail {

/// The format of the trpc standard caller/callee name is: trpc.app.server.service
/// @brief The index of app filed after splitting by '.'
constexpr std::size_t kAppIndex = 1;
/// @brief The index of server filed after splitting by '.'
constexpr std::size_t kServerIndex = 2;
/// @brief The index of service filed after splitting by '.'
constexpr std::size_t kServiceIndex = 3;

/// @brief Splits a string into substrings based on a specified separator character.
/// @param str The caller/callee name
/// @param separator The separator character
/// @param [out] output A vector used to store the results of splitting
/// @note The original data in the output will be cleared. If the str is split into more than four segments, all content
///       starting from the fourth segment will be treated as the fourth segment.
void SplitName(const std::string& str, char separator, std::vector<std::string>& output);

/// @brief Gets the service name from str. It will return the part 4 of the split result.
/// @param str The caller/callee name
/// @param separator The separator character
/// @return service name
/// @note If the str is split into more than four segments, all content starting from the fourth segment will be treated
///       as the fourth segment. If the str is split into less than four segments, it will returns an empty string.
std::string GetServiceName(const std::string& str, char separator);

/// @brief Sets the caller service info using the filed split by service_name
/// @param service_name caller service name
/// @param [out] infos used to store information
void SetCallerServiceInfo(const std::string& service_name, std::map<std::string, std::string>& infos);

/// @brief Sets the callee service info using the filed split by service_name
/// @param service_name callee service name
/// @param [out] infos used to store information
void SetCalleeServiceInfo(const std::string& service_name, std::map<std::string, std::string>& infos);

}  // namespace detail

}  // namespace trpc::prometheus
#endif
