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

#include <mutex>
#include <string>
#include <unordered_map>

#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/tvar/compound_ops/latency_recorder.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::stream {

/// @brief Streaming RPC metric counter.
class StreamVar : public RefCounted<StreamVar> {
 public:
  explicit StreamVar(const std::string& var_path);

  ~StreamVar() = default;

  /// @brief Increases the issued RPC call count by |count|.
  void AddRpcCallCount(size_t count) { rpc_call_count_.Add(count); }

  /// @brief Increases the failed RPC call count by |count|.
  void AddRpcCallFailureCount(size_t count) { rpc_call_failure_count_.Add(count); }

  /// @brief Increases the message send count by |count|.
  void AddSendMessageCount(size_t count) { send_msg_count_.Add(count); }

  /// @brief Increases the message received count by |count|.
  void AddRecvMessageCount(size_t count) { recv_msg_count_.Add(count); }

  /// @brief Increases the message send bytes by |bytes|.
  void AddSendMessageBytes(size_t bytes) { send_msg_bytes_.Add(bytes); }

  /// @brief Increases the message received bytes by |bytes|.
  void AddRecvMessageBytes(size_t bytes) { recv_msg_bytes_.Add(bytes); }

  /// @brief Returns the issued RPC call count.
  /// @note Inefficient operation, not recommended for frequent use.
  uint64_t GetRpcCallCountValue(std::string* var_path = nullptr) {
    if (var_path) {
      *var_path = rpc_call_count_.GetAbsPath();
    }
    return rpc_call_count_.GetValue();
  }

  /// @brief Returns the failed RPC call count.
  /// @note Inefficient operation, not recommended for frequent use.
  uint64_t GetRpcCallFailureCountValue(std::string* var_path = nullptr) {
    if (var_path) {
      *var_path = rpc_call_failure_count_.GetAbsPath();
    }
    return rpc_call_failure_count_.GetValue();
  }

  /// @brief Returns the message send count.
  /// @note Inefficient operation, not recommended for frequent use.
  uint64_t GetSendMessageCountValue(std::string* var_path = nullptr) {
    if (var_path) {
      *var_path = send_msg_count_.GetAbsPath();
    }
    return send_msg_count_.GetValue();
  }

  /// @brief Returns the the message received count.
  /// @note Inefficient operation, not recommended for frequent use.
  uint64_t GetRecvMessageCountValue(std::string* var_path = nullptr) {
    if (var_path) {
      *var_path = recv_msg_count_.GetAbsPath();
    }
    return recv_msg_count_.GetValue();
  }

  /// @brief Returns the message send bytes.
  /// @note Inefficient operation, not recommended for frequent use.
  uint64_t GetSendMessageBytesValue(std::string* var_path = nullptr) {
    if (var_path) {
      *var_path = send_msg_bytes_.GetAbsPath();
    }
    return send_msg_bytes_.GetValue();
  }

  /// @brief Returns the message received bytes.
  /// @note Inefficient operation, not recommended for frequent use.
  uint64_t GetRecvMessageBytesValue(std::string* var_path = nullptr) {
    if (var_path) {
      *var_path = recv_msg_bytes_.GetAbsPath();
    }
    return recv_msg_bytes_.GetValue();
  }

  /// @brief Collect snapshot of streaming metric counter.
  void CaptureStreamVarSnapshot(std::unordered_map<std::string, uint64_t>* snapshot);

  /// @brief Gets streaming counter metrics.
  void GetStreamVarMetrics(std::unordered_map<std::string, uint64_t>* metrics);

 private:
  // RPC call count.
  tvar::Counter<uint64_t> rpc_call_count_;

  // RPC call failure count.
  tvar::Counter<uint64_t> rpc_call_failure_count_;

  // Message send count (the count of 'DATA' frame).
  tvar::Counter<uint64_t> send_msg_count_;

  // Message received count (the count of 'DATA' frame).
  tvar::Counter<uint64_t> recv_msg_count_;

  // Message send bytes.
  tvar::Counter<uint64_t> send_msg_bytes_;

  // Message received bytes.
  tvar::Counter<uint64_t> recv_msg_bytes_;

  // Snapshot of streaming metric counter (key: metric_var_path, value: metric_value).
  std::unordered_map<std::string, uint64_t> stream_var_snapshot_;
};
using StreamVarPtr = RefPtr<StreamVar>;

StreamVarPtr CreateStreamVar(const std::string& var_path);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// @brief A helper for streaming metric counter.
class StreamVarHelper {
 public:
  static StreamVarHelper* GetInstance() {
    static StreamVarHelper instance;
    return &instance;
  }

  ~StreamVarHelper();

  StreamVarHelper(const StreamVarHelper&) = delete;
  StreamVarHelper& operator=(const StreamVarHelper&) = delete;

  /// @brief Gets streaming counter based on counter path, creates one if it doesn't exist.
  StreamVarPtr GetOrCreateStreamVar(const std::string& var_path);

  /// @brief Reports streaming statistical data.
  bool ReportStreamVarMetrics(const std::string& metrics_name);

  /// @brief Reports streaming statistical data.
  bool ReportMetrics(const std::string& metrics_name, const std::unordered_map<std::string, uint64_t>& metrics);

  /// @brief Increases active streaming counter.
  void IncrementActiveStreamCount();

  /// @brief Decreases active streaming counter.
  void DecrementActiveStreamCount();

  /// @brief Returns the active streaming counter.
  /// @note Inefficient operation, not recommended for frequent use.
  uint64_t GetActiveStreamCount();

 private:
  StreamVarHelper();

 private:
  std::mutex mutex_;
  std::unordered_map<std::string, StreamVarPtr> stream_vars_;
  // Active streaming.
  tvar::Gauge<int64_t> active_stream_count_;
};

}  // namespace trpc::stream
