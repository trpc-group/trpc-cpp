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

#include <cstdint>
#include <memory>
#include <vector>

#include "trpc/transport/client/future/common/future_connector.h"

namespace trpc {

/// @brief Connection pool implementation for pipeline scenario.
///        Distinguished from ConnPool, PipelineConnPool no need to support
///        shared pending queue as requests are sent immediately as pipeline,
///        also, PipelineConnPool select connection in round-robin manner,
///        which is far different from ConnPool. That is why ConnPool is
///        not reused in pipeline scenario.
template <typename T>
class PipelineConnPool {
 public:
  static_assert(std::is_base_of_v<FutureConnector, T>);

  /// @brief Constructor.
  explicit PipelineConnPool(uint64_t max_conn_num);

  /// @brief Destructor.
  ~PipelineConnPool();

  /// @brief Stop connection pool.
  void Stop();

  // @brief Destroy connection pool.
  void Destroy();

  /// @brief Get a available connector id in connection pool.
  /// @return uint64_t Connector id.
  uint64_t GenAvailConnectorId();

  /// @brief Get connector by connector id.
  /// @param conn_id Connector id.
  /// @return Connector object.
  T* GetConnector(uint64_t conn_id);

  /// @brief Add a new connector into connection pool.
  /// @param conn_id Connector id.
  /// @param conn Connector object.
  /// @return true: success, false: failed.
  bool AddConnector(uint64_t conn_id, std::unique_ptr<T>&& conn);

  /// @brief Get connector by connector id and delete it from connection pool.
  std::unique_ptr<T> GetAndDelConnector(uint64_t conn_id);

  /// @brief Get all the connectors in connection pool.
  const std::vector<std::unique_ptr<T>>& GetConnectors() { return connectors_; }

 private:
  // Max connector number.
  uint64_t max_conn_num_;

  // All the connectors.
  std::vector<std::unique_ptr<T>> connectors_;

  // For round-robin selection.
  uint64_t connector_index_{0};
};

template <typename T>
PipelineConnPool<T>::PipelineConnPool(uint64_t max_conn_num) : max_conn_num_(max_conn_num) {
  connectors_.reserve(max_conn_num_);
  for (uint64_t i = 0; i < max_conn_num_; i++) {
    connectors_.emplace_back(nullptr);
  }
}

template <typename T>
PipelineConnPool<T>::~PipelineConnPool() {}

template <typename T>
void PipelineConnPool<T>::Stop() {
  for (auto& connector : connectors_) {
    if (connector) {
      connector->Stop();
    }
  }
}

template <typename T>
void PipelineConnPool<T>::Destroy() {
  for (auto& connector : connectors_) {
    if (connector) {
      connector->Destroy();
    }
  }
}

template <typename T>
uint64_t PipelineConnPool<T>::GenAvailConnectorId() {
  uint64_t index = connector_index_++ % max_conn_num_;
  return index;
}

template <typename T>
T* PipelineConnPool<T>::GetConnector(uint64_t conn_id) {
  return connectors_[conn_id].get();
}

template <typename T>
bool PipelineConnPool<T>::AddConnector(uint64_t conn_id, std::unique_ptr<T>&& conn) {
  connectors_[conn_id] = std::move(conn);
  return true;
}

template <typename T>
std::unique_ptr<T> PipelineConnPool<T>::GetAndDelConnector(uint64_t conn_id) {
  auto connector = std::move(connectors_[conn_id]);
  return connector;
}

}  // namespace trpc
