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

#include "gtest/gtest.h"

#include "trpc/filter/filter_manager.h"
#include "trpc/rpcz/filter/server_filter.h"

namespace trpc::testing {

TEST(SetSampleFunction, SetServerSampleFunctionTest) {
  // It is safe to DelServerSampleFunction without rpz filter set.
  trpc::rpcz::DelServerSampleFunction();

  // Register rpcz filter.
  MessageServerFilterPtr rpzc_filter(new trpc::rpcz::RpczServerFilter());
  rpzc_filter->Init();
  bool add_result = trpc::FilterManager::GetInstance()->AddMessageServerFilter(rpzc_filter);
  ASSERT_TRUE(add_result);

  // Get rpcz filter.
  trpc::rpcz::RpczServerFilter* rpcz_filter = nullptr;
  auto filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter("rpcz");
  if (filter != nullptr && static_cast<trpc::rpcz::RpczServerFilter*>(filter.get()) != nullptr) {
    rpcz_filter = static_cast<trpc::rpcz::RpczServerFilter*>(filter.get());
  }

  trpc::ServerContextPtr context;

  auto rpcz_sample_succ_func = [](const trpc::ServerContextPtr& ctx) { return true; };
  trpc::rpcz::SetServerSampleFunction(rpcz_sample_succ_func);
  ASSERT_EQ(rpcz_filter->GetSampleFunction()(context), true);

  auto rpcz_sample_fail_func = [](const trpc::ServerContextPtr& ctx) { return false; };
  trpc::rpcz::SetServerSampleFunction(rpcz_sample_fail_func);
  ASSERT_EQ(rpcz_filter->GetSampleFunction()(context), false);

  // Delete sample function now.
  trpc::rpcz::DelServerSampleFunction();
  ASSERT_EQ(rpcz_filter->GetSampleFunction(), nullptr);
}

TEST(SetSampleFunction, SetClientSampleFunctionTest) {
  // It is safe to DelClientSampleFunction without rpz filter set.
  trpc::rpcz::DelClientSampleFunction();

  // Register client rpcz filter.
  MessageClientFilterPtr rpzc_filter(new trpc::rpcz::RpczClientFilter());
  rpzc_filter->Init();
  bool add_result = trpc::FilterManager::GetInstance()->AddMessageClientFilter(rpzc_filter);
  ASSERT_TRUE(add_result);

  // Get client rpcz filter.
  trpc::rpcz::RpczClientFilter* rpcz_filter = nullptr;
  auto filter = trpc::FilterManager::GetInstance()->GetMessageClientFilter("rpcz");
  if (filter != nullptr && static_cast<trpc::rpcz::RpczClientFilter*>(filter.get()) != nullptr) {
    rpcz_filter = static_cast<trpc::rpcz::RpczClientFilter*>(filter.get());
  }

  trpc::ClientContextPtr context;

  // Verify do sample.
  auto rpcz_sample_succ_func = [](const trpc::ClientContextPtr& ctx) { return true; };
  trpc::rpcz::SetClientSampleFunction(rpcz_sample_succ_func);
  ASSERT_EQ(rpcz_filter->GetSampleFunction()(context), true);

  // verify do not sample.
  auto rpcz_sample_fail_func = [](const trpc::ClientContextPtr& ctx) { return false; };
  trpc::rpcz::SetClientSampleFunction(rpcz_sample_fail_func);
  ASSERT_EQ(rpcz_filter->GetSampleFunction()(context), false);

  // Delete sample function now.
  trpc::rpcz::DelClientSampleFunction();
  ASSERT_EQ(rpcz_filter->GetSampleFunction(), nullptr);
}

}  // namespace trpc::testing
#endif
