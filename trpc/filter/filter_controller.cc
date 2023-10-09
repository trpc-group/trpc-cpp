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

#include "trpc/filter/filter_controller.h"

namespace trpc {

bool FilterController::AddMessageServerFilter(const MessageServerFilterPtr& filter) {
  return server_filter_controller_.AddMessageServerFilter(filter);
}

bool FilterController::AddMessageClientFilter(const MessageClientFilterPtr& filter) {
  return client_filter_controller_.AddMessageClientFilter(filter);
}

FilterStatus FilterController::RunMessageServerFilters(FilterPoint type,
                                                       const ServerContextPtr& context) {
  return server_filter_controller_.RunMessageServerFilters(type, context);
}

FilterStatus FilterController::RunMessageClientFilters(FilterPoint type,
                                                       const ClientContextPtr& context) {
  return client_filter_controller_.RunMessageClientFilters(type, context);
}

}  // namespace trpc
