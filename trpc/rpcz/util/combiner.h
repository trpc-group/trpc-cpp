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

#include <atomic>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "trpc/rpcz/util/block_group.h"
#include "trpc/rpcz/util/link_list.h"
#include "trpc/util/log/logging.h"

namespace trpc::rpcz {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/detail/combiner.h

/// @brief Add const & to non integral type.
/// @note Should be merged with tvar add_cr_non_integral_t, duplicate code here.
/// @private
template <typename T>
struct AddConstNonIntegral {
  using type = std::conditional_t<std::is_integral_v<T>, T, std::add_lvalue_reference_t<std::add_const_t<T>>>;
};

/// @brief Helper for AddConstNonIntegral.
/// @private
template <typename T>
using add_cr_non_integral_t = typename AddConstNonIntegral<T>::type;

/// @brief Just call operation with two values as parameters.
/// @note Overdesigned code here.
/// @private
template <typename Op, typename T1, typename T2>
void CallOpReturningVoid(const Op& op, T1& v1, const T2& v2) {
  return op(v1, v2);
}

/// @brief Make operations of type T thread safe.
/// @note Should be merged with tvar AtomicType, duplicate code here.
/// @private
template <typename T>
class ElementContainer {
 public:
  /// @brief Get value, return by parameter.
  void Load(T* out) {
    std::lock_guard<std::mutex> guard(mutex_);
    *out = value_;
  }

  /// @brief Write new value.
  void Store(const T& new_value) {
    std::lock_guard<std::mutex> guard(mutex_);
    value_ = new_value;
  }

  /// @brief Write new value and return old value.
  void Exchange(T* prev, const T& new_value) {
    std::lock_guard<std::mutex> guard(mutex_);
    *prev = value_;
    value_ = new_value;
  }

  /// @brief Modify value through user operation.
  template <typename Op, typename T1>
  void Modify(const Op& op, const T1& value2) {
    std::lock_guard<std::mutex> guard(mutex_);
    CallOpReturningVoid(op, value_, value2);
  }

 private:
  // Raw value to make it thread safe.
  T value_;
  // Lock to protect the value.
  mutable std::mutex mutex_;
};

/// @brief Let thread local block merged into global block list.
/// @private
template <typename ResultType, typename ElementType, typename BinaryOp>
class BlockCombiner {
 public:
  using SelfType = BlockCombiner<ResultType, ElementType, BinaryOp>;

  /// @brief Type of thread local block.
  struct Block : public LinkNode<Block> {
    /// @brief Default constructor.
    Block() : combiner_(nullptr) {}

    /// @brief Destructor.
    ~Block() {
      if (combiner_) {
        combiner_->CommitAndErase(this);
        combiner_ = nullptr;
      }
    }

    /// @brief To init current block.
    void Reset(const ElementType& val, SelfType* self_type_ptr) {
      combiner_ = self_type_ptr;
      element_.Store(val);
    }

    // Upper combiner current block belongs to.
    SelfType* combiner_;
    // Current block value.
    ElementContainer<ElementType> element_;
  };

  using RpczBlockGroup = BlockGroup<Block>;

  /// @brief Constructor.
  explicit BlockCombiner(const ResultType& result_identity = ResultType(),
                         const ElementType& element_identity = ElementType(),
                         const BinaryOp& op = BinaryOp())
      : id_(0), op_(op), global_result_(result_identity), element_identity_(element_identity) {}

  /// @brief Destructor.
  ~BlockCombiner() {
    if (id_ >= 0) {
      ClearAllBlock();
      id_ = -1;
    }
  }

  /// @brief Merge all thread local blocks and get result.
  ResultType ResetAllBlocks() {
    ElementType prev;
    std::lock_guard<std::mutex> lock(mutex_);
    ResultType reset_result = nullptr;
    for (LinkNode<Block>* node = blocks_.head(); node != blocks_.end(); node = node->next()) {
      node->value()->element_.Exchange(&prev, element_identity_);
      CallOpReturningVoid(op_, reset_result, prev);
    }
    return reset_result;
  }

  /// @brief Merge block value into global result and remove block from list.
  void CommitAndErase(Block* block) {
    if (block == nullptr) {
      return;
    }
    ElementType local;
    std::lock_guard<std::mutex> lock(mutex_);
    block->element_.Load(&local);
    CallOpReturningVoid(op_, global_result_, local);
    block->RemoveFromList();
  }

  /// @brief Get thread local block, create it if not found.
  Block* GetOrCreateTlsBlock() {
    Block* block = RpczBlockGroup::GetTlsBlock(id_);
    if (!block) {
      block = RpczBlockGroup::GetOrCreateTlsBlock(id_);
      if (nullptr == block) {
        TRPC_FMT_ERROR("Fail to create block");
        return nullptr;
      }
    }

    if (block->combiner_) {
      return block;
    }

    block->Reset(element_identity_, this);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      blocks_.Append(block);
    }
    return block;
  }

  /// @brief Clear all blocks from global blocks list.
  void ClearAllBlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (LinkNode<Block>* node = blocks_.head(); node != blocks_.end();) {
      node->value()->Reset(ElementType(), nullptr);
      LinkNode<Block>* const saved_next = node->next();
      node->RemoveFromList();
      node = saved_next;
    }
  }

  /// @brief Get operation.
  const BinaryOp& GetOp() const { return op_; }

 private:
  // block id.
  uint16_t id_;
  // Operation to merge thread local block.
  BinaryOp op_;
  // To protect global blocks.
  std::mutex mutex_;
  // Merged result.
  ResultType global_result_;
  // Initial value for each block.
  ElementType element_identity_;
  // Global blocks.
  LinkedList<Block> blocks_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::rpcz
#endif
