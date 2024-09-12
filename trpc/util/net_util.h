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

#include <cstdint>
#include <string>

namespace trpc::util {

/// @brief Ip transfer(int to string)
std::string Ipv4ToString(uint32_t ip, bool* ok = nullptr);

/// @brief Ip transfer(string to int)
uint32_t StringToIpv4(const std::string& ip, bool* ok = nullptr);

/// @brief Parse host and port
/// @param name[in] str
/// Format support:
/// 1. ipv4:port(eg: 127.0.0.1:90)
/// 2. [ipv6]:port(eg: [::1]:90)
/// 3. domain:port(eg: www.baidu.com:8080)
/// @param host[out] ip or domain
/// @param port[out] port
/// @param is_ipv6[out] whether is ip v6
/// @param weight[out] weight
/// @return True: success, False: failed
bool ParseHostPort(const std::string& name, std::string& host, int& port, bool& is_ipv6, uint32_t& weight);

/// @brief Get ip by eth name
std::string GetIpByEth(std::string_view eth_inf);

/// @brief Generate a random available port
///        for testing
int GenRandomAvailablePort(uint32_t retry_num = 10);

/// @brief When the kernel enables tcp_autocorking, even if the `TCP_CORK` option
///        is disabled on the socket, data may still be sent delayed.
///        If you want the data to be sent immediately, you must execute this function
///        once after the data is written into the buffer by send/writev.
void FlushTcpCorkedData(int sock_fd);

/// @brief Ignore pipe signal
void IgnorePipe();

/// @brief Determine if a string is a valid IP address.
/// @param ip Input string
/// @param is_ipv6 Is an IPv6 address or not
/// @return If input is a valid IP address returns true, otherwise returns false.
bool IsValidIp(const std::string& ip, bool is_ipv6);

/// @brief Check if the input string is in the format of IP:port combinations,
///        with multiple IP:port pairs separated by commas, such as 127.0.0.1:80,127.0.0.1:8080.
///        If the IP address is IPv6, the IP should be enclosed in square brackets, such as [ip]:port.
/// @param ip_ports Input string
/// @return If input is in the format of IP:port combinations returns true, otherwise returns false.
bool IsValidIpPorts(const std::string& ip_ports);

}  // namespace trpc::util
