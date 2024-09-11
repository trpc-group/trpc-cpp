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

#include "trpc/util/net_util.h"

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <random>
#include <utility>

#include "trpc/util/log/logging.h"
#include "trpc/util/string_util.h"

namespace trpc::util {

namespace {

int GenRandomInt(int start, int end) {
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(start, end);

  return uniform_dist(e1);
}

// Check whether the port can be bound to avoid the port being occupied
bool IsPortAvailable(int port) {
  int sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = port;

  int ret = ::bind(sock, (struct sockaddr*)&(addr), sizeof(struct sockaddr_in));
  if (ret != 0) {
    close(sock);
    return false;
  }

  close(sock);

  return true;
}

}  // namespace

std::string Ipv4ToString(uint32_t ip, bool *ok) {
  struct in_addr src;
  src.s_addr = ip;

  char dst[INET_ADDRSTRLEN];
  if (!inet_ntop(AF_INET, &src, dst, sizeof(dst))) {
    strerror_r(errno, dst, sizeof(dst));
    TRPC_LOG_ERROR(dst);
    if (ok) *ok = false;
    return "";
  }
  if (ok) *ok = true;
  return dst;
}

uint32_t StringToIpv4(const std::string &ip, bool *ok) {
  struct in_addr dst;
  int ret = inet_pton(AF_INET, ip.data(), &dst);
  if (ret == 0) {
    TRPC_LOG_ERROR("Not in presentation format");
    if (ok) *ok = false;
    return 0;
  } else if (ret < 0) {
    char err[20];
    strerror_r(errno, err, sizeof(err));
    TRPC_LOG_ERROR(err);
    if (ok) *ok = false;
    return 0;
  }

  if (ok) *ok = true;
  return dst.s_addr;
}

bool ParseHostPort(const std::string& name, std::string& host, int& port, bool& is_ipv6, uint32_t& weight) {
  std::string temp_host;
  int temp_port;
  bool temp_is_ipv6;
  uint32_t temp_weight = 0;
  size_t weight_start;

  if (!name.empty() && name[0] == '[') {
    // ipv6
    const size_t rbracket = name.find(']', 1);
    if (rbracket == std::string::npos) {
      // [ipv6]:port missing ']'
      return false;
    }

    const size_t colons = rbracket + 1;
    if (colons >= name.size() - 1) {
      // [ipv6]:port colons position invalid
      return false;
    }

    if (name[colons] != ':') {
      // [ipv6]:port missing ':'
      return false;
    }

    temp_host = name.substr(1, rbracket - 1);
    if (temp_host.find(':') == std::string::npos) {
      // ipv6 invalid
      return false;
    }
    try {
      weight_start = name.find("(", colons);
      temp_port = std::stoi(name.substr(colons + 1, weight_start - (colons + 1)));
    } catch (...) {
      // port invalid
      return false;
    }
    temp_is_ipv6 = true;
  } else {
    // ipv4 or domain
    size_t colon = name.find(':');
    if (colon == std::string::npos || colon >= name.size() - 1 || name.find(':', colon + 1) != std::string::npos) {
      // not found port or format error
      return false;
    }
    temp_host = name.substr(0, colon);
    weight_start = name.find("(", colon);
    try {
      temp_port = std::stoi(name.substr(colon + 1, weight_start - (colon + 1)));
    } catch (...) {
      // port invalid
      return false;
    }
    temp_is_ipv6 = false;
  }

  size_t weight_end = name.find(')', weight_start);
  if (weight_start != std::string::npos && weight_end != std::string::npos && weight_start < weight_end) {
    try {
      temp_weight = std::stoul(name.substr(weight_start + 1, weight_end - weight_start - 1));  
    } catch (...) {
      return false;
    }
  }
  host = std::move(temp_host);
  port = temp_port;
  is_ipv6 = temp_is_ipv6;
  weight = temp_weight;
  return true;
}

std::string GetIpByEth(std::string_view eth_inf) {
  std::string str;

  struct sockaddr_in sin;
  struct ifreq ifr;

  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  TRPC_ASSERT(sd > 0);

  strncpy(ifr.ifr_name, eth_inf.data(), IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = 0;

  // if error: No such device
  if (::ioctl(sd, SIOCGIFADDR, &ifr) < 0){
    TRPC_LOG_ERROR("ioctl error: " << strerror(errno) << " eth: " << eth_inf);
    ::close(sd);
    return str;
  }

  ::memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
  str = inet_ntoa(sin.sin_addr);

  ::close(sd);
  return str;
}

int GenRandomAvailablePort(uint32_t retry_num) {
  for (uint32_t i = 0; i < retry_num; i++) {
    int port = GenRandomInt(10000, 60000);
    if (IsPortAvailable(port)) {
      return port;
    }
  }

  return -1;
}

void FlushTcpCorkedData(int sock_fd) {
  int zero = 0;
  ::setsockopt(sock_fd, IPPROTO_TCP, TCP_CORK, &zero, sizeof(zero));
}

void IgnorePipe() {
  struct sigaction sig;

  memset(&sig, 0, sizeof(struct sigaction));

  sig.sa_handler = SIG_IGN;
  sig.sa_flags = 0;
  sigemptyset(&sig.sa_mask);
  sigaction(SIGPIPE, &sig, NULL);
}

bool IsValidIp(const std::string& ip, bool is_ipv6) {
  if (!is_ipv6) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
  } else {
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, ip.c_str(), &(sa.sin6_addr)) != 0;
  }

  return false;
}

bool IsValidIpPorts(const std::string& ip_ports) {
  std::vector<std::string> vec = util::SplitString(ip_ports, ',');
  std::string host;
  int port;
  bool is_ipv6;
  uint32_t weight;
  for (auto const& item : vec) {
    if (!util::ParseHostPort(item, host, port, is_ipv6, weight)) {
      return false;
    }

    // check ip
    if (!IsValidIp(host, is_ipv6)) {
      return false;
    }

    // check port
    if (port < 0 || port > 65535) {
      return false;
    }
  }
  return true;
}

}  // namespace trpc::util
