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

#include "trpc/util/domain_util.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <regex>
#include <set>
#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc::util {

AddrType GetAddrType(const std::string& addr) {
  // ipv4
  static const char* regex_ipv4 =
      R"(^((25[0-5]|(2[0-4]|1[0-9]|[1-9]){1}[0-9]|[1-9])\.)((25[0-5]|(2[0-4]|1[0-9]|[1-9]){0,1}[0-9])\.){2}(25[0-5]|(2[0-4]|1[0-9]|[1-9]){0,1}[0-9])$)";  // NOLINT

  // ipv6
  static const char* regex_ipv6 =
      R"((([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fAF]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fAF]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fAF]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9afA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fAF]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zAZ]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])))";  // NOLINT

  // domain
  static const char* regex_domain =
      R"(^(?=^.{3,255}$)[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})*(\.[a-zA-Z][-a-zA-Z]{0,62}){1}(\.)?$)";

  if (std::regex_match(addr, std::regex(regex_ipv4))) {
    return AddrType::ADDR_IP;
  }

  if (std::regex_match(addr, std::regex(regex_ipv6))) {
    return AddrType::ADDR_IP;
  }

  std::string str = addr;
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  if (std::regex_match(addr, std::regex(regex_domain)) || str.compare("localhost") == 0) {
    return AddrType::ADDR_DOMAIN;
  }

  return AddrType::ADDR_NX;
}

void GetAddrFromDomain(const std::string& domain, std::vector<std::string>& addrs) {
  addrs.clear();

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_ADDRCONFIG;
  hints.ai_protocol = 0;

  struct addrinfo* res = NULL;
  int ret = getaddrinfo(domain.c_str(), NULL, &hints, &res);
  if (ret != 0) {
    const char* str = gai_strerror(ret);
    TRPC_LOG_ERROR("getaddrinfo failed, domain:" << domain << ",ret:" << ret
                                                 << ",strerror:" << (str ? str : ""));
    return;
  }

  struct addrinfo* res_p = NULL;
  std::set<std::string> unrepeatable_ip;  // ip deduplication
  for (res_p = res; res_p != NULL; res_p = res_p->ai_next) {
    if (res_p->ai_family == AF_INET) {
      struct sockaddr_in* addr = (struct sockaddr_in*)(res_p->ai_addr);
      char addr_buf[INET_ADDRSTRLEN] = {0};
      const char* addr_ptr = inet_ntop(AF_INET, &addr->sin_addr, addr_buf, sizeof(addr_buf));
      if (!addr_ptr) {
        TRPC_LOG_ERROR("ipv4 inet_ntop failed, domain:" << domain << ",errno:" << errno);
        continue;
      }
      unrepeatable_ip.emplace(addr_ptr);
    } else if (res_p->ai_family == AF_INET6) {
      struct sockaddr_in6* addr = (struct sockaddr_in6*)(res_p->ai_addr);
      char addr_buf[INET6_ADDRSTRLEN] = {0};
      const char* addr_ptr = inet_ntop(AF_INET6, &addr->sin6_addr, addr_buf, sizeof(addr_buf));
      if (!addr_ptr) {
        TRPC_LOG_ERROR("ipv6 inet_ntop failed, domain:" << domain << ",errno:" << errno);
        continue;
      }
      unrepeatable_ip.emplace(addr_ptr);
    } else {
      TRPC_LOG_ERROR("domain:" << domain << ",unknown ai_family:" << res_p->ai_family);
      continue;
    }
  }
  freeaddrinfo(res);

  addrs.reserve(unrepeatable_ip.size());
  while (!unrepeatable_ip.empty()) {
    addrs.emplace_back(std::move(unrepeatable_ip.extract(unrepeatable_ip.begin()).value()));
  }
}

}  // namespace trpc::util
