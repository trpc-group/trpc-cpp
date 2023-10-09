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

#include "trpc/tvar/common/atomic_type.h"
#include "trpc/tvar/common/op_util.h"

namespace trpc::tvar {

/// @brief Provide uniform class and interface to WriteMostly.
/// @private
template <typename T, typename InputDataT, typename ResultT, typename UpdateOpT,
          typename MergeOpT, typename SamplerOpT, typename SamplerInvOpT>
struct BasicTraits {
  using Type = T;
  using WriteBuffer = AtomicType<T>;
  using InputDataType = InputDataT;
  using ResultType = ResultT;
  using SamplerOp = SamplerOpT;
  using SamplerInvOp = SamplerInvOpT;

  template <typename TLS>
  static void Update(TLS* wb, InputDataType val) {
    wb->buffer_.template Modify<UpdateOpT, InputDataType>(val);
  }

  static void Merge(ResultType* wb1, add_cr_non_integral_t<Type> wb2) {
    MergeOpT()(wb1, wb2);
  }
};

}  // namespace trpc::tvar
