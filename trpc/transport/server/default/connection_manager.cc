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

#include "trpc/transport/server/default/connection_manager.h"

#include <cstdint>

#include "trpc/util/log/logging.h"

namespace trpc {

namespace {

uint32_t FindTableSizeof2(const uint32_t target) {
  TRPC_ASSERT(target < INT32_MAX);

  uint32_t temp = target;
  temp |= temp >> 1;
  temp |= temp >> 2;
  temp |= temp >> 4;
  temp |= temp >> 8;
  temp |= temp >> 16;
  return temp + 1;
}

}  // namespace

ConnectionManager::IdGenerator::IdGenerator(uint32_t max_item_size) : max_item_size_(max_item_size) {
  for (uint32_t i = 1; i <= max_item_size_; ++i) {
    free_.push(i);
  }

  base_num_ = FindTableSizeof2(max_item_size);
}

bool ConnectionManager::IdGenerator::Get(uint32_t& id) {
  if (!free_.empty()) {
    id = free_.front();
    free_.pop();
    return true;
  }

  return false;
}

void ConnectionManager::IdGenerator::Recycle(uint32_t id) {
  uint64_t next_id = id;
  next_id += base_num_;
  if (TRPC_UNLIKELY(next_id > UINT32_MAX)) {
    next_id = id & (base_num_ - 1);
    TRPC_ASSERT(next_id <= max_item_size_);
  }

  free_.push(next_id);
}

ConnectionManager::ConnectionManager(uint32_t reactor_id, uint32_t max_conn_num)
    : reactor_id_(reactor_id),
      max_conn_num_(max_conn_num < kMaxConnNumLimit ? max_conn_num : kMaxConnNumLimit),
      id_generator_(max_conn_num_) {
  if (max_conn_num > kMaxConnNumLimit) {
    TRPC_FMT_ERROR("Max conn num {} is too big, we set it to {}", max_conn_num, kMaxConnNumLimit);
  }

  list_.resize(max_conn_num_ + 1);
  for (uint64_t i = 1; i <= max_conn_num_; ++i) {
    list_[i].Reset();
  }

  base_num_ = id_generator_.GetBaseNum();
}

void ConnectionManager::Stop() {
  for (uint64_t i = 1; i <= max_conn_num_; ++i) {
    if (list_[i]) {
      list_[i]->DoClose(false);
    }
  }
}

void ConnectionManager::Destroy() {
  for (uint64_t i = 1; i <= max_conn_num_; ++i) {
    if (list_[i]) {
      DelConnection(list_[i]->GetConnId());
    }
  }
  list_.clear();
}

void ConnectionManager::DisableAllConnRead() {
  for (uint64_t i = 1; i <= max_conn_num_; ++i) {
    if (list_[i]) {
      list_[i]->DisableRead();
    }
  }
}

uint64_t ConnectionManager::GenConnectionId() {
  if (cur_conn_num_ >= max_conn_num_) {
    return 0;
  }

  uint32_t uid = 0;
  if (id_generator_.Get(uid)) {
    ++cur_conn_num_;
    return (0xFFFFFFFF00000000 & (static_cast<uint64_t>(reactor_id_) << 32)) | uid;
  }

  return 0;
}

bool ConnectionManager::AddConnection(const RefPtr<TcpConnection>& conn) {
  auto conn_id = conn->GetConnId();
  TRPC_ASSERT(((0xFFFFFFFF00000000 & conn_id) >> 32) == reactor_id_);

  auto uid = (0x00000000FFFFFFFF & conn_id) & (base_num_ - 1);
  TRPC_ASSERT(uid <= max_conn_num_);
  if (list_[uid].Get() != nullptr) {
    return false;
  }

  list_[uid] = conn;

  return true;
}

TcpConnection* ConnectionManager::GetConnection(uint64_t conn_id) {
  TRPC_ASSERT(((0xFFFFFFFF00000000 & conn_id) >> 32) == reactor_id_);
  auto uid = (0x00000000FFFFFFFF & conn_id) & (base_num_ - 1);
  TRPC_ASSERT(uid <= max_conn_num_);
  auto& conn = list_[uid];
  if (conn != nullptr && conn->GetConnId() == conn_id) {
    return conn.Get();
  }

  return nullptr;
}

RefPtr<TcpConnection> ConnectionManager::DelConnection(uint64_t conn_id) {
  TRPC_ASSERT(((0xFFFFFFFF00000000 & conn_id)) >> 32 == reactor_id_);

  auto uid = (0x00000000FFFFFFFF & conn_id);
  id_generator_.Recycle(uid);

  uid = uid & (base_num_ - 1);

  TRPC_ASSERT(uid <= max_conn_num_);
  RefPtr<TcpConnection> ptr = std::move(list_[uid]);

  --cur_conn_num_;

  return ptr;
}

}  // namespace trpc
