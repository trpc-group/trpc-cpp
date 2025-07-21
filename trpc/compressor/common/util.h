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

#pragma once

#include "trpc/util/buffer/zero_copy_stream.h"

namespace trpc::compressor {

/// @brief Copy bytes from |data| to |out| buffer.
/// @return Returns true on success, false otherwise.
bool CopyDataToOutputStream(const void* data, std::size_t size, trpc::NoncontiguousBufferOutputStream* out);

}  // namespace trpc::compressor
