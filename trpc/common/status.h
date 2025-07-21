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

#include <string>
#include <utility>

namespace trpc {

/// @brief The status information class returned by the RPC call.
/// @note  If an error occurs in the main process of the framework, the framework status code will be
///        set to non-zero value.
///        Function execution status code is used as the return value of the function.
///        Meanwhile, the error message can also be set.

class Status {
 public:
  Status() : ret_(0), func_ret_(0) {}

  explicit Status(int func_ret, std::string error_message)
      : ret_(0), func_ret_(func_ret), error_message_(std::move(error_message)) {}

  explicit Status(int ret, int func_ret, std::string error_message)
      : ret_(ret), func_ret_(func_ret), error_message_(std::move(error_message)) {}

  /// @brief  Set the return error code of the framework.
  /// @param  ret int type. code of the framework
  void SetFrameworkRetCode(int ret) { ret_ = ret; }

  /// @brief Set the return error code of the RPC method execution.
  /// @param func_ret int type. code of the RPC method
  /// @note Error code (`func_ret`) representing business processing.
  void SetFuncRetCode(int func_ret) { func_ret_ = func_ret; }

  /// @brief Set error message.
  /// @param error_message string type. detail error message
  void SetErrorMessage(std::string error_message) { error_message_ = std::move(error_message); }

  /// @brief Set the error code of the framework.
  /// @return int type
  int GetFrameworkRetCode() const { return ret_; }

  /// @brief Set the error code of the RPC method execution.
  /// @return int type
  int GetFuncRetCode() const { return func_ret_; }

  /// @brief Get error message
  /// @return string type
  const std::string& ErrorMessage() const { return error_message_; }

  /// @brief Check if the RPC call is successful.
  /// @return bool type
  bool OK() const { return ret_ == 0 && func_ret_ == 0; }

  /// @brief This method is specifically designed for use with streaming RPC
  /// @note Currently, -99 is reserved as the stream EOF code, which does not conflict with the error codes in
  ///       `trpc.proto`.
  /// @return bool type
  bool StreamEof() const { return ret_ == -99; }

  /// @brief This method is specifically designed for use with streaming RPC
  /// @note Currently, -97 is reserved as the stream RST code, which does not conflict with the error codes in
  ///       `trpc.proto`.
  /// @return bool type
  bool StreamRst() const { return ret_ == -97; }

  /// @brief Return a string representation of this status suitable for printing.
  /// @return Returns the string "OK" for success.
  std::string ToString() const;

 private:
  int ret_;
  int func_ret_;
  std::string error_message_;
};

inline const Status kDefaultStatus{0, ""};
inline const Status kSuccStatus{0, ""};
inline const Status kUnknownErrorStatus{-99999, -99999, "unknown error"};
inline const Status kStreamRstStatus{-97, 0, "stream reset"};

}  // namespace trpc
