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
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "trpc/transport/client/future/common/future_connector.h"
#include "trpc/transport/client/future/common/future_connector_options.h"
#include "trpc/transport/client/future/common/timingwheel_timeout_queue.h"
#include "trpc/transport/client/future/common/utils.h"
#include "trpc/util/time.h"

namespace trpc {

/// @brief Connection pool to manage a group of connectors.
template <typename T>
class ConnPool {
 public:
  static_assert(std::is_base_of_v<FutureConnector, T>);

  /// @brief Identify invalid connector id.
  static constexpr uint64_t kInvalidConnId = std::numeric_limits<uint64_t>::max();

  /// @brief Constructor.
  explicit ConnPool(const FutureConnectorGroupOptions& options);

  /// @brief Destructor.
  ~ConnPool() = default;

  /// @brief Stop connection pool.
  void Stop();

  /// @brief Destroy connection pool.
  void Destroy();

  /// @brief Get a available connector id in connection pool.
  /// @note  Different connection pools may have the same connector id.
  uint64_t GenAvailConnectorId();

  /// @brief Recycle connector id for further assignment.
  /// @param conn_id Connector id.
  void RecycleConnectorId(uint64_t conn_id);

  /// @brief Add a new connector into connection pool.
  /// @param conn_id Connector id to identify the connector.
  /// @param conn Connector object.
  /// @return true: success, false: failed.
  bool AddConnector(uint64_t conn_id, std::unique_ptr<T>&& conn);

  /// @brief Get connector by connector id.
  /// @param conn_id Connector id.
  /// @return Connector object.
  T* GetConnector(uint64_t conn_id);

  /// @brief Delete connector by connector id.
  /// @param conn_id Connector id.
  void DelConnector(uint64_t conn_id);

  /// @brief Get connector by connector id and delete it from connection pool.
  /// @param conn_id Connector id.
  std::unique_ptr<T> GetAndDelConnector(uint64_t conn_id);

  /// @brief Get all the connectors in connection pool.
  const std::vector<std::unique_ptr<T>>& GetConnectors() { return connectors_; }

  /// @brief Check there is no established connection in connection pool or not.
  /// @return true: no established connection, false: at least one established connection.
  bool IsEmpty() { return max_conn_num_ == free_.size(); }

  /// @brief Get the number of free connector.
  int SizeOfFree() { return free_.size(); }

  /// @brief Add request message into pending timeout queue.
  /// @param req_msg Request message.
  /// @return true: success, false: failed.
  bool PushToPendingQueue(CTransportReqMsg* req_msg);

  /// @brief Pop one request from pending timeout queue.
  CTransportReqMsg* PopFromPendingQueue();

  /// @brief Handle request timeout in pending timeout queue.
  void HandlePendingQueueTimeout();

 private:
  // Limit the max size of pending timeout queue.
  constexpr static size_t kPendingSendQueueSize = 10000;

  // Group options.
  FutureConnectorGroupOptions options_;

  // Max connector number.
  uint64_t max_conn_num_;

  // Queue of connector id.
  std::deque<uint64_t> free_;

  // Whether specialized connector id is inside free_ queue.
  std::vector<bool> is_in_;

  // All the pending requests resides in this queue is of non-fixed-connection scenario.
  // In connection pool scenario, when all the connections has request running, new
  // coming requests have to wait in this pending queue until connection available.
  internal::SmallCacheTimingWheelTimeoutQueue pending_queue_;

  // Function for request timeout handling in pending queue.
  Function<void(const internal::SmallCacheTimingWheelTimeoutQueue::DataIterator&)> timeout_handle_function_;

  // All the connectors.
  std::vector<std::unique_ptr<T>> connectors_;
};

template <typename T>
ConnPool<T>::ConnPool(const FutureConnectorGroupOptions& options)
    : options_(options), max_conn_num_(options.trans_info->max_conn_num) {
  connectors_.resize(max_conn_num_);
  is_in_.resize(max_conn_num_);

  // Connector id starts from 0.
  for (uint64_t i = 0; i < max_conn_num_; ++i) {
    connectors_[i] = nullptr;
    free_.push_back(i);
    is_in_[i] = true;
  }

  timeout_handle_function_ = [this](const internal::SmallCacheTimingWheelTimeoutQueue::DataIterator& iter) {
    CTransportReqMsg* msg = this->pending_queue_.GetAndPop(iter);

    if (msg->extend_info->backup_promise) {
      Exception ex(CommonException("resend pending timeout", TRPC_CLIENT_INVOKE_TIMEOUT_ERR));
      future::NotifyBackupRequestResend(std::move(ex), msg->extend_info->backup_promise);

      BackupRequestRetryInfo* retry_info_ptr = msg->context->GetBackupRequestRetryInfo();
      TRPC_ASSERT(msg->context->GetTimeout() > retry_info_ptr->delay);
      msg->context->SetTimeout(msg->context->GetTimeout() - retry_info_ptr->delay);
      PushToPendingQueue(msg);
      return;
    }

    std::string error = "future invoke timeout before send, peer addr:";
    error += msg->context->GetIp().c_str();
    error += ":";
    error += std::to_string(msg->context->GetPort());
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, std::move(error),
                              options_.trans_info->rsp_dispatch_function);
  };
}

template <typename T>
uint64_t ConnPool<T>::GenAvailConnectorId() {
  // All the connector ids is assigned.
  if (free_.empty()) {
    return kInvalidConnId;
  }

  uint64_t conn_id = *(free_.rbegin());

  // Now this connector id is assgined, but connector is not into vector yet.
  free_.pop_back();
  is_in_[conn_id] = false;

  return conn_id;
}

template <typename T>
void ConnPool<T>::RecycleConnectorId(uint64_t conn_id) {
  // Only when connector id is assigned but connector not into vector.
  if (!is_in_[conn_id]) {
    is_in_[conn_id] = true;
    free_.push_back(conn_id);
  }
}

template <typename T>
bool ConnPool<T>::AddConnector(uint64_t conn_id, std::unique_ptr<T>&& conn) {
  // Already inside, conflict.
  if (connectors_[conn_id] != nullptr) {
    return false;
  }

  connectors_[conn_id] = std::move(conn);

  return true;
}

template <typename T>
T* ConnPool<T>::GetConnector(uint64_t conn_id) {
  return connectors_[conn_id].get();
}

template <typename T>
void ConnPool<T>::DelConnector(uint64_t conn_id) {
  auto connector = GetAndDelConnector(conn_id);
  if (connector) {
    connector->Stop();
    connector->Destroy();
  }
}

template <typename T>
std::unique_ptr<T> ConnPool<T>::GetAndDelConnector(uint64_t conn_id) {
  RecycleConnectorId(conn_id);
  std::unique_ptr<T> connector = std::move(connectors_[conn_id]);
  return connector;
}

template <typename T>
bool ConnPool<T>::PushToPendingQueue(CTransportReqMsg* req_msg) {
  size_t size = pending_queue_.Size();
  if (size > kPendingSendQueueSize) {
    std::string err = "Client Overload in Pending Queue";
    future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR, std::move(err),
                              options_.trans_info->rsp_dispatch_function);
    return false;
  }

  int64_t timeout = req_msg->context->GetTimeout();
  if (req_msg->extend_info->backup_promise) {
    BackupRequestRetryInfo* retry_info_ptr = req_msg->context->GetBackupRequestRetryInfo();
    timeout = retry_info_ptr->delay;
  }
  timeout += trpc::time::GetMilliSeconds();

  if (pending_queue_.Push(req_msg->context->GetRequestId(), req_msg, timeout)) return true;

  // Push failed.
  std::string err = "Client Request Push to Pending Queue Failed, Maybe Seq Id is conflict, id = ";
  err += req_msg->context->GetRequestId();
  future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, std::move(err),
                            options_.trans_info->rsp_dispatch_function);
  return false;
}

template <typename T>
CTransportReqMsg* ConnPool<T>::PopFromPendingQueue() {
  return pending_queue_.Pop();
}

template <typename T>
void ConnPool<T>::HandlePendingQueueTimeout() {
  uint64_t now_ms = trpc::time::GetMilliSeconds();
  pending_queue_.DoTimeout(now_ms, timeout_handle_function_);
}

template <typename T>
void ConnPool<T>::Stop() {
  CTransportReqMsg* msg = nullptr;
  while ((msg = pending_queue_.Pop()) != nullptr) {
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, "network error",
                              options_.trans_info->rsp_dispatch_function);
  }

  for (uint64_t i = 0; i < max_conn_num_; ++i) {
    if (connectors_[i]) {
      connectors_[i]->Stop();
    }
  }
}

template <typename T>
void ConnPool<T>::Destroy() {
  for (uint64_t i = 0; i < max_conn_num_; ++i) {
    if (connectors_[i]) {
      connectors_[i]->Destroy();
      connectors_[i] = nullptr;
    }
  }
}

}  // namespace trpc
