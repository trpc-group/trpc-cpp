//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/demangle.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/demangle.h"

#include <cxxabi.h>

#include <string>

#include "trpc/util/deferred.h"

namespace trpc::internal {

std::string Demangle(const char* s) {
  [[maybe_unused]] int status;
  auto demangled = abi::__cxa_demangle(s, nullptr, nullptr, &status);
  ScopedDeferred _([&] { free(demangled); });
  if (!demangled) {
    return s;
  }
  return demangled;
}

}  // namespace trpc::internal
