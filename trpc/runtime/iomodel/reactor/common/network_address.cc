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

#include "trpc/runtime/iomodel/reactor/common/network_address.h"

#include <tuple>
#include <utility>

namespace trpc {

NetworkAddress::NetworkAddress(const struct sockaddr* addr) {
  switch (addr->sa_family) {
    case AF_INET: {
      addr_ = *reinterpret_cast<const sockaddr_in *>(addr);
      type_ = IpType::kIpV4;
      break;
    }
    case AF_INET6: {
      addr_ = *reinterpret_cast<const sockaddr_in6 *>(addr);
      type_ = IpType::kIpV6;
      break;
    }
    default:
      break;
  }
}

NetworkAddress::NetworkAddress(uint16_t port, bool loopback, IpType type) {
  type_ = type;
  switch (type_) {
    case IpType::kIpV4: {
      struct sockaddr_in ipv4_addr;
      ipv4_addr.sin_family = AF_INET;
      ipv4_addr.sin_port = htons(port);
      ipv4_addr.sin_addr.s_addr = loopback ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
      addr_ = ipv4_addr;
      break;
    }
    case IpType::kIpV6: {
      struct sockaddr_in6 ipv6_addr;
      ipv6_addr.sin6_family = AF_INET6;
      ipv6_addr.sin6_port = htons(port);
      ipv6_addr.sin6_addr = loopback ? in6addr_loopback : in6addr_any;
      ipv6_addr.sin6_flowinfo = 0;
      ipv6_addr.sin6_scope_id = 0;
      addr_ = ipv6_addr;
    }
    default:
      break;
  }
}

/// ipv4 format: xxx.xxx.xxx.xxx
/// ipv6 format: xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
NetworkAddress::NetworkAddress(std::string_view ip, uint16_t port, IpType type) {
  if (type == IpType::kUnknown) {
    type = DecideIpType(ip);
  }

  type_ = type;
  switch (type_) {
    case IpType::kIpV4: {
      addr_ = FromIpv4Str(ip, port);
      break;
    }
    case IpType::kIpV6: {
      addr_ = FromIpv6Str(ip, port);
      break;
    }
    default:
      break;
  }
}

NetworkAddress::NetworkAddress(std::string_view ip_port, IpType type) {
  switch (type) {
    case IpType::kIpV4: {
      auto pos = ip_port.find(":");
      if (pos != std::string_view::npos) {
        std::string_view ip = ip_port.substr(0, pos);
        std::string_view port = ip_port.substr(pos + 1);

        addr_ = FromIpv4Str(ip, static_cast<uint16_t>(atoi(port.data())));
        type_ = IpType::kIpV4;
      }
      break;
    }
    case IpType::kIpV6: {
      if (ip_port.empty() || ip_port[0] != '[') {
        break;
      }

      auto pos = ip_port.find("]:");
      if (pos != std::string_view::npos) {
        std::string_view ip = ip_port.substr(1, pos - 1);
        std::string_view port = ip_port.substr(pos + 2);

        addr_ = FromIpv6Str(ip, static_cast<uint16_t>(atoi(port.data())));
        type_ = IpType::kIpV6;
      }
      break;
    }
    default:
      break;
  }
}

bool NetworkAddress::IsValid() const { return type_ != IpType::kUnknown; }

std::string NetworkAddress::ToString() const {
  switch (type_) {
    case IpType::kIpV4: {
      return Ip() + ":" + std::to_string(Port());
      break;
    }
    case IpType::kIpV6: {
      return "[" + Ip() + "]:" + std::to_string(Port());
      break;
    }
    default:
      break;
  }
  return "";
}

std::string NetworkAddress::Ip() const {
  std::string str;
  switch (type_) {
    case IpType::kIpV4: {
      if (auto addr = std::get_if<struct sockaddr_in>(&addr_)) {
        char buf[INET_ADDRSTRLEN];
        str = inet_ntop(AF_INET, reinterpret_cast<const void *>(&addr->sin_addr),
                        buf, INET_ADDRSTRLEN);
      }
      break;
    }
    case IpType::kIpV6: {
      if (auto addr = std::get_if<struct sockaddr_in6>(&addr_)) {
        char buf[INET6_ADDRSTRLEN];
        str = inet_ntop(AF_INET6, reinterpret_cast<const void *>(&addr->sin6_addr),
                        buf, INET6_ADDRSTRLEN);
      }
      break;
    }
    default:
      break;
  }
  return str;
}

uint16_t NetworkAddress::Port() const {
  switch (type_) {
    case IpType::kIpV4: {
      if (auto addr = std::get_if<struct sockaddr_in>(&addr_)) {
        return ntohs(addr->sin_port);
      }
      break;
    }
    case IpType::kIpV6: {
      if (auto addr = std::get_if<struct sockaddr_in6>(&addr_)) {
        return ntohs(addr->sin6_port);
      } else {
        return 0;
      }
      break;
    }
    default:
      break;
  }
  return 0;
}

NetworkAddress::IpType NetworkAddress::Type() const { return type_; }

sa_family_t NetworkAddress::Family() const { return static_cast<sa_family_t>(Type()); }

bool NetworkAddress::IsIpv4() const { return type_ == IpType::kIpV4; }

bool NetworkAddress::IsIpv6() const { return type_ == IpType::kIpV6; }

NetworkAddress::operator bool() const { return type_ != IpType::kUnknown; }

struct sockaddr_in NetworkAddress::FromIpv4Str(std::string_view ip, uint16_t port) {
  struct sockaddr_in addr;
  if (inet_pton(AF_INET, std::string(ip).c_str(), &(addr.sin_addr)) <= 0) {
    type_ = IpType::kUnknown;
    addr.sin_family = AF_UNSPEC;
    return addr;
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  type_ = IpType::kIpV4;
  return addr;
}

struct sockaddr_in6 NetworkAddress::FromIpv6Str(std::string_view ip, uint16_t port) {
  struct sockaddr_in6 addr;
  if (inet_pton(AF_INET6, std::string(ip).c_str(), &(addr.sin6_addr)) <= 0) {
    type_ = IpType::kUnknown;
    addr.sin6_family = AF_UNSPEC;
    return addr;
  }
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(port);
  addr.sin6_flowinfo = 0;
  addr.sin6_scope_id = 0;
  type_ = IpType::kIpV6;
  return addr;
}

struct sockaddr* NetworkAddress::SockAddr() {
  switch (type_) {
    case IpType::kIpV4: {
      return reinterpret_cast<struct sockaddr*>(std::get_if<struct sockaddr_in>(&addr_));
    }
    case IpType::kIpV6: {
      return reinterpret_cast<struct sockaddr*>(std::get_if<struct sockaddr_in6>(&addr_));
    }
    default:
      return nullptr;
  }
}

const struct sockaddr* NetworkAddress::SockAddr() const {
  switch (type_) {
    case IpType::kIpV4: {
      return reinterpret_cast<const struct sockaddr *>(std::get_if<struct sockaddr_in>(&addr_));
    }
    case IpType::kIpV6: {
      return reinterpret_cast<const struct sockaddr *>(std::get_if<struct sockaddr_in6>(&addr_));
    }
    default:
      return nullptr;
  }
}

socklen_t NetworkAddress::Socklen() const {
  return IsIpv6() ? static_cast<socklen_t>(sizeof(struct sockaddr_in6))
                  : static_cast<socklen_t>(sizeof(struct sockaddr_in));
}

NetworkAddress::IpType NetworkAddress::DecideIpType(std::string_view ip) {
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;
  if (inet_pton(AF_INET, ip.data(), &(addr4.sin_addr)) > 0) {
    return IpType::kIpV4;
  } else if (inet_pton(AF_INET6, ip.data(), &(addr6.sin6_addr)) > 0) {
    return IpType::kIpV6;
  } else {
    return IpType::kUnknown;
  }
}

bool operator==(const NetworkAddress &l, const NetworkAddress &r) {
  return std::forward_as_tuple(l.Ip(), l.Port()) == std::forward_as_tuple(r.Ip(), r.Port());
}

bool operator!=(const NetworkAddress &l, const NetworkAddress &r) { return !(l == r); }

bool operator<(const NetworkAddress &l, const NetworkAddress &r) {
  return std::forward_as_tuple(l.Ip(), l.Port()) < std::forward_as_tuple(r.Ip(), r.Port());
}

}  // namespace trpc
