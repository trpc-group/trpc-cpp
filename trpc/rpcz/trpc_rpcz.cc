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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/trpc_rpcz.h"

#include <ostream>

#include "trpc/filter/filter_manager.h"
#include "trpc/rpcz/collector.h"
#include "trpc/rpcz/span.h"
#include "trpc/util/string_util.h"

namespace trpc::rpcz {

void SetServerSampleFunction(const CustomerServerRpczSampleFunction& sample_function) {
  auto filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter("rpcz");
  if (filter != nullptr) {
    auto* rpcz_filter = static_cast<RpczServerFilter*>(filter.get());
    rpcz_filter->SetSampleFunction(sample_function);
  } else {
    TRPC_FMT_ERROR("not set rpcz filter in framwork server config, please check it");
  }
}

void DelServerSampleFunction() {
  auto filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter("rpcz");
  // Server rpcz filter not registered, just return.
  if (filter == nullptr) {
    return;
  }
  auto* rpcz_filter = static_cast<RpczServerFilter*>(filter.get());
  rpcz_filter->DelSampleFunction();
}

void SetClientSampleFunction(const CustomerClientRpczSampleFunction& sample_function) {
  auto filter = trpc::FilterManager::GetInstance()->GetMessageClientFilter("rpcz");
  if (filter != nullptr) {
    auto* rpcz_filter = static_cast<RpczClientFilter*>(filter.get());
    rpcz_filter->SetSampleFunction(sample_function);
  } else {
    TRPC_FMT_ERROR("not set rpcz filter in framwork client config, please check it");
  }
}

void DelClientSampleFunction() {
  auto filter = trpc::FilterManager::GetInstance()->GetMessageClientFilter("rpcz");
  // Client rpcz filter not registered, just return.
  if (filter == nullptr) {
    return;
  }
  auto* rpcz_filter = static_cast<RpczClientFilter*>(filter.get());
  rpcz_filter->DelSampleFunction();
}

}  // namespace trpc::rpcz
#endif
