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

#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Manager class for connection
class ConnectionManager {
 public:
  /// The generator for connection id
  /// In the asynchronous packet return scenario:
  ///   after the connection id is recycled, this id may be assigned to a new connection,
  ///   resulting in the packet return in error connection.
  /// Therefore, the id generator has been designed as follows:
  ///   in order to ensure that the id allocated next time is different from the previous one,
  ///   an additional integer (integer multiple of the base) will be added to the id each time it is recycled,
  ///   recycle until id exceeds UINT32_MAX.
  /// In order to be more efficient, the base number is the power of 2 that is closest to and greater than
  /// the maximum number of connections.
  class IdGenerator {
   public:
    explicit IdGenerator(uint32_t max_item_size);

    // Returns true when there is an available id,
    // and returns false when there is no available id
    bool Get(uint32_t& id);

    // Recycle id
    void Recycle(uint32_t id);

    inline uint32_t GetBaseNum() { return base_num_; }

   private:
    // max item size
    uint32_t max_item_size_;

    // The power of 2 closest to and greater than max_item_size_
    uint32_t base_num_{0};

    // available id queue
    std::queue<uint32_t> free_;
  };

  explicit ConnectionManager(uint32_t reactor_id, uint32_t max_conn_num);

  /// @brief Stop connection
  void Stop();

  /// @brief Destroy connection
  void Destroy();

  /// @brief Get available connection id
  uint64_t GenConnectionId();

  /// @brief Add connection
  bool AddConnection(const RefPtr<TcpConnection>& conn);

  /// @brief Get connection by id
  TcpConnection* GetConnection(uint64_t connection_id);

  /// @brief Del connection by id
  RefPtr<TcpConnection> DelConnection(uint64_t connection_id);

  /// @brief close all connections read event
  void DisableAllConnRead();

 private:
  // max num of connections one thread
  static constexpr uint32_t kMaxConnNumLimit = 1000000;

  // reactor id
  uint32_t reactor_id_;

  // The maximum number of connections set by the user
  uint32_t max_conn_num_{100000};

  // current number of connections
  uint32_t cur_conn_num_ = 0;

  // connection list
  std::vector<RefPtr<TcpConnection>> list_;

  // Object for allocating and deallocating connection ids
  IdGenerator id_generator_;

  // The power of 2 closest to and greater than max_conn_num_
  uint32_t base_num_{0};
};

}  // namespace trpc
