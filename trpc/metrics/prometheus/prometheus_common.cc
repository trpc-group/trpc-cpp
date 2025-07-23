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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/metrics/prometheus/prometheus_common.h"

#include <algorithm>
#include <string_view>

namespace trpc::prometheus {

namespace detail {

void SplitName(const std::string& str, char separator, std::vector<std::string>& output) {
  output.clear();
  std::size_t cur_index = 0;  // the index of the current segment being processed.
  std::size_t start_pos = 0, next_pos = 0;
  for (; start_pos < str.size(); start_pos = next_pos + 1) {
    next_pos = str.find(separator, start_pos);
    if (next_pos == std::string::npos || cur_index++ == kServiceIndex) {
      output.emplace_back(str.substr(start_pos));
      return;
    }
    output.emplace_back(str.substr(start_pos, next_pos - start_pos));
  }
}

std::string GetServiceName(const std::string& str, char separator) {
  std::size_t cur_index = 0;
  std::size_t pos = 0;
  while ((pos = str.find(separator, pos)) != std::string::npos) {
    if (++cur_index == kServiceIndex) {
      return str.substr(pos + 1);
    }
    ++pos;
  }
  return "";
}

void SetCallerServiceInfo(const std::string& service_name, std::map<std::string, std::string>& infos) {
  std::vector<std::string> sv;
  sv.reserve(4);
  trpc::prometheus::detail::SplitName(service_name, '.', sv);
  if (sv.size() > 3) {
    infos.emplace(trpc::prometheus::kCallerAppKey, std::move(sv[1]));
    infos.emplace(trpc::prometheus::kCallerServerKey, std::move(sv[2]));
    infos.emplace(trpc::prometheus::kCallerServiceKey, std::move(sv[3]));
  } else if (sv.size() > 2) {
    infos.emplace(trpc::prometheus::kCallerAppKey, std::move(sv[1]));
    infos.emplace(trpc::prometheus::kCallerServerKey, std::move(sv[2]));
    infos.emplace(trpc::prometheus::kCallerServiceKey, "");
  } else if (sv.size() > 1) {
    infos.emplace(trpc::prometheus::kCallerAppKey, std::move(sv[1]));
    infos.emplace(trpc::prometheus::kCallerServerKey, "");
    infos.emplace(trpc::prometheus::kCallerServiceKey, "");
  } else {
    infos.emplace(trpc::prometheus::kCallerAppKey, service_name);
    infos.emplace(trpc::prometheus::kCallerServerKey, "");
    infos.emplace(trpc::prometheus::kCallerServiceKey, "");
  }
}

void SetCalleeServiceInfo(const std::string& service_name, std::map<std::string, std::string>& infos) {
  std::vector<std::string> sv;
  sv.reserve(4);
  trpc::prometheus::detail::SplitName(service_name, '.', sv);
  if (sv.size() > 3) {
    infos.emplace(trpc::prometheus::kCalleeAppKey, std::move(sv[1]));
    infos.emplace(trpc::prometheus::kCalleeServerKey, std::move(sv[2]));
    infos.emplace(trpc::prometheus::kCalleeServiceKey, std::move(sv[3]));
  } else if (sv.size() > 2) {
    infos.emplace(trpc::prometheus::kCalleeAppKey, std::move(sv[1]));
    infos.emplace(trpc::prometheus::kCalleeServerKey, std::move(sv[2]));
    infos.emplace(trpc::prometheus::kCalleeServiceKey, "");
  } else if (sv.size() > 1) {
    infos.emplace(trpc::prometheus::kCalleeAppKey, std::move(sv[1]));
    infos.emplace(trpc::prometheus::kCalleeServerKey, "");
    infos.emplace(trpc::prometheus::kCalleeServiceKey, "");
  } else {
    infos.emplace(trpc::prometheus::kCalleeAppKey, service_name);
    infos.emplace(trpc::prometheus::kCalleeServerKey, "");
    infos.emplace(trpc::prometheus::kCalleeServiceKey, "");
  }
}

}  // namespace detail

}  // namespace trpc::prometheus
#endif
