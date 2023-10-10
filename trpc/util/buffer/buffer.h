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

#include "trpc/util/buffer/contiguous_buffer.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @note The following interface is only allowed to be used internally within the framework.

/// @brief Converting the original contiguous buffer in the framework to a NoncontiguousBuffer buffer. no copy
/// @param contiguous_buffer The BufferPtr that needs to be converted.
/// @param noncontiguous_buffer The NoncontiguousBuffer after conversion.
/// @return Bool type, True: success; Fale: failed
/// @note The converted buffer no longer supports the cut operation.
/// @private For internal use purpose only.
bool ContiguousToNonContiguous(BufferPtr& contiguous_buffer, NoncontiguousBuffer& noncontiguous_buffer);

/// @brief Converting the NoncontiguousBuffer to the original BufferPtr in the framework. it will copy
/// @param noncontiguous_buffer The NoncontiguousBuffer that needs to be converted.
/// @param contiguous_buffer The BufferPtr after conversion.
/// @return Bool type, True: success; Fale: failed
/// @private For internal use purpose only.
bool NonContiguousToContiguous(const NoncontiguousBuffer& noncontiguous_buffer, BufferPtr& contiguous_buffer);

}  // namespace trpc
