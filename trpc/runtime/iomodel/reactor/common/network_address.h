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

#include <arpa/inet.h>

#include <string>
#include <string_view>
#include <variant>

namespace trpc {

/// @brief Encapsulation for network address family(ip v4/v6)
class NetworkAddress {
 public:
  enum class IpType { kIpV4 = AF_INET, kIpV6 = AF_INET6, kUnknown = AF_UNSPEC };

  NetworkAddress() = default;

  explicit NetworkAddress(const struct sockaddr* addr);

  explicit NetworkAddress(uint16_t port, bool loopback, IpType type = IpType::kIpV4);

  /// @brief Ipv4 format: xxx.xxx.xxx.xxx
  ///        Ipv6 format: xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
  explicit NetworkAddress(std::string_view ip, uint16_t port, IpType type = IpType::kIpV4);

  /// @brief Ipv4 format: xxx.xxx.xxx.xxx:yyy
  ///        Ipv6 format: [xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]:yyy
  explicit NetworkAddress(std::string_view ip_port, IpType type = IpType::kIpV4);

  bool IsValid() const;

  std::string ToString() const;

  std::string Ip() const;

  uint16_t Port() const;

  IpType Type() const;

  sa_family_t Family() const;

  bool IsIpv4() const;

  bool IsIpv6() const;

  struct sockaddr* SockAddr();

  const struct sockaddr* SockAddr() const;

  socklen_t Socklen() const;

  explicit operator bool() const;

  friend bool operator==(const NetworkAddress &l, const NetworkAddress &r);

  friend bool operator!=(const NetworkAddress &l, const NetworkAddress &r);

  friend bool operator<(const NetworkAddress &l, const NetworkAddress &r);

 private:
  IpType DecideIpType(std::string_view ip);
  struct sockaddr_in FromIpv4Str(std::string_view ip, uint16_t port);
  struct sockaddr_in6 FromIpv6Str(std::string_view ip, uint16_t port);

  IpType type_{IpType::kUnknown};
  std::variant<struct sockaddr_in, struct sockaddr_in6> addr_;
};

}  // namespace trpc

namespace std {

template <>
struct hash<trpc::NetworkAddress> {
  std::size_t operator()(const trpc::NetworkAddress &addr) const {
    switch (addr.Type()) {
      case trpc::NetworkAddress::IpType::kIpV4:
      case trpc::NetworkAddress::IpType::kIpV6:
        return std::hash<std::string>()(addr.Ip()) ^ std::hash<uint16_t>()(addr.Port());
      default:
        return 0;
    }
  }
};

}  // namespace std
