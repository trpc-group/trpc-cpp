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

#include <memory>
#include <string>
#include <type_traits>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/status.h"

namespace trpc {

/// @brief Base class for Exception.
class ExceptionBase {
 public:
  virtual ~ExceptionBase() = default;

  /// @brief Overrided by subclass to return a customized error string.
  virtual const char *what() const noexcept = 0;

  void SetExceptionCode(int ret_code) { ret_code_ = ret_code; }

  /// @brief  Get error code of exception.
  int GetExceptionCode() { return ret_code_; }

 protected:
  int ret_code_;
  std::string msg_;
};

/// @brief Common exception.
class CommonException : public ExceptionBase {
 public:
  explicit CommonException(const char *msg, int ret_code = TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR) {
    ret_code_ = ret_code;
    msg_ = std::string(msg);
  }

  const char *what() const noexcept override { return msg_.c_str(); }
};

/// @brief Unary rpc error exception, for async future invoke
class UnaryRpcError : public ExceptionBase {
 public:
  UnaryRpcError() : status_(kUnknownErrorStatus) { FillExceptionCode(); }
  explicit UnaryRpcError(Status&& status) : status_(std::move(status)) { FillExceptionCode(); }
  explicit UnaryRpcError(const Status& status) : status_(status) { FillExceptionCode(); }

  const char* what() const noexcept override {
    if (what_.empty()) what_ = status_.ToString();
    return what_.data();
  }

  const Status& GetStatus() const { return status_; }

 private:
  void FillExceptionCode() {
    ExceptionBase::SetExceptionCode(status_.GetFrameworkRetCode() != 0 ? status_.GetFrameworkRetCode()
                                                                       : status_.GetFuncRetCode());
  }
  Status status_;
  mutable std::string what_;
};

class Exception final {
 public:
  /// @brief Explicitly retain default constructor.
  Exception() = default;

  /// @brief Explicitly retain destructor.
  ~Exception() = default;

  /// @brief Explicitly retain move constructor.
  Exception(Exception &&e) = default;

  /// @brief Explicitly retain copy constructor.
  Exception(const Exception &e) = default;

  /// @brief Explicitly retain move = operator.
  Exception &operator=(Exception &&e) = default;

  /// @brief Explicitly retain copy = operator.
  Exception &operator=(const Exception &e) = default;

  /// @brief Specialization for copy = operator by subclass of ExceptionBase.
  template <typename ExceptionType, typename =
                                    typename std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value>::type>
  Exception &operator=(ExceptionType e) {
    ptr = std::make_shared<ExceptionType>(e);
    return *this;
  }

  /// @brief Specialization for constructor by subclass of ExceptionBase.
  template <typename ExceptionType, typename =
                                    typename std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value>::type>
  Exception(ExceptionType e) {
    ptr = std::make_shared<ExceptionType>(e);
  }

  /// @brief Get exception string massage.
  const char *what() const {
    if (ptr == nullptr) {
      return "empty exception";
    }
    return ptr->what();
  }

  /// @brief get exception code.
  int GetExceptionCode() {
    if (ptr == nullptr) {
      return TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR;
    }
    return ptr->GetExceptionCode();
  }

  /// @brief Check the exception object is ExceptionType or not.
  /// @tparam ExceptionType
  /// @tparam std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value>::type
  /// @return true If the exception object is ExceptionType or derived from ExceptionType, false Otherwise.
  template <typename ExceptionType, typename =
                                    typename std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value>::type>
  bool is() const {
    if (ptr == nullptr) {
      return false;
    }
    if (dynamic_cast<ExceptionType *>(ptr.get()) != nullptr) {
      return true;
    }
    return false;
  }

  /// @brief Same as above.
  /// @tparam ExceptionType
  /// @tparam std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value>::type
  /// @param e ExceptionType, keep for format.
  /// @return true If the exception object is ExceptionType or derived from ExceptionType, false Otherwise.
  template <typename ExceptionType,
            typename = typename std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value>::type>
  bool is(const ExceptionType& e) const {
    if (ptr == nullptr) {
      return false;
    }
    if (dynamic_cast<ExceptionType *>(ptr.get()) != nullptr) {
      return true;
    }
    return false;
  }

  /// @brief Get inner exception while is<ExceptionType> is true.
  /// @tparam ExceptionType
  /// @return Shared pointer to inner ExceptionType object.
  template <typename ExceptionType>
  std::shared_ptr<ExceptionType> Get() const {
    return std::dynamic_pointer_cast<ExceptionType>(ptr);
  }

 private:
  std::shared_ptr<ExceptionBase> ptr = nullptr;
};

}  // namespace trpc
