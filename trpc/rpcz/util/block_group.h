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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ

#pragma once

#include <stdlib.h>

#include <deque>
#include <mutex>
#include <new>
#include <string>
#include <vector>

#include "trpc/util/align.h"
#include "trpc/util/check.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::rpcz {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/detail/agent_group.h

/// @brief Hash table of block.
/// @private
template <typename Block>
class BlockGroup {
 public:
  static constexpr uint16_t kRawBlockSize = 4096;
  static constexpr uint16_t kElementsPreBlock = (kRawBlockSize + sizeof(Block) - 1) / sizeof(Block);

  /// @brief Array of block.
  struct alignas(trpc::hardware_constructive_interference_size) ThreadBlock {
    Block* at(uint16_t offset) { return block_vec_ + offset; }

   private:
    Block block_vec_[kElementsPreBlock];
  };

  /// @brief Get thread local block through id.
  static Block* GetTlsBlock(uint16_t id) {
    if (TRPC_LIKELY(id >= 0)) {
      if (tls_blocks_) {
        const uint16_t block_id = id / kElementsPreBlock;
        if (block_id < tls_blocks_->size()) {
          ThreadBlock* const tb = (*tls_blocks_)[block_id];
          if (tb) {
            return tb->at(id - block_id * kElementsPreBlock);
          }
        }
      }
    }
    return nullptr;
  }

  /// @brief Get thread local block through id, create it if not found.
  static Block* GetOrCreateTlsBlock(uint16_t id) {
    if (TRPC_UNLIKELY(id < 0)) {
      TRPC_CHECK(false, "Invalid id={}", id);
      return nullptr;
    }

    if (tls_blocks_ == nullptr) {
      tls_blocks_ = new (std::nothrow) std::vector<ThreadBlock*>;
      if (TRPC_UNLIKELY(tls_blocks_ == nullptr)) {
        TRPC_FMT_ERROR("create tls_blocks_ failed");
        return nullptr;
      }
      std::atexit(DestroyTlsBlocks);
    }

    const size_t block_id = id / kElementsPreBlock;
    if (block_id >= tls_blocks_->size()) {
      tls_blocks_->resize(std::max(block_id + 1, 32ul));
    }

    ThreadBlock* tb = (*tls_blocks_)[block_id];
    if (tb == nullptr) {
      ThreadBlock* new_block = new (std::nothrow) ThreadBlock;
      if (TRPC_UNLIKELY(new_block == nullptr)) {
        return nullptr;
      }

      tb = new_block;
      (*tls_blocks_)[block_id] = new_block;
    }

    TRPC_LOG_DEBUG("block id:" << block_id << ", sub id:" << id - block_id * kElementsPreBlock
                               << ", kElementsPreBlock:" << kElementsPreBlock << ", block_size:" << (sizeof(Block)));

    return tb->at(id - block_id * kElementsPreBlock);
  }

 private:
  /// @brief Destroy all thread local blocks.
  static void DestroyTlsBlocks() {
    if (!tls_blocks_) {
      return;
    }

    for (size_t i = 0; i < tls_blocks_->size(); ++i) {
      delete (*tls_blocks_)[i];
    }

    delete tls_blocks_;
    tls_blocks_ = nullptr;
  }

 private:
  static thread_local std::vector<ThreadBlock*>* tls_blocks_;
};

template <typename Block>
thread_local std::vector<typename BlockGroup<Block>::ThreadBlock*>* BlockGroup<Block>::tls_blocks_ = nullptr;

// End of source codes that are from incubator-brpc.

}  // namespace trpc::rpcz
#endif
