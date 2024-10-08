//
// Created by kosmos on 10/7/24.
//

#pragma once

#include "trpc/client/mysql/executor/mysql_executor.h"

namespace trpc::mysql {

class TransactionHandle {

 public:
  enum class TxState {kNotInited, kStart, kEnd, kInValid};

  explicit TransactionHandle(RefPtr<MysqlExecutor> &&executor) : executor_(std::move(executor)) {}

  TransactionHandle() :executor_(nullptr) {}

  TransactionHandle& operator=(TransactionHandle&& other) noexcept {
    state_ = other.state_;
    executor_ = std::move(other.executor_);
    other.state_ = TxState::kInValid;
    other.executor_ = nullptr;
    return *this;
  }

  TransactionHandle& operator=(const TransactionHandle& other) = delete;



  ~TransactionHandle() {
    if(executor_)
      executor_->Close();
    state_ = TxState::kInValid;
  }

  void SetState(TxState state) { state_ = state; }

  TxState GetState() { return state_; }

  RefPtr<MysqlExecutor> GetExecutor() { return executor_; }

  RefPtr<MysqlExecutor>&& TransferExecutor() { return std::move(executor_); }

 private:
  RefPtr<MysqlExecutor> executor_{nullptr};
  TxState state_{TxState::kNotInited};
};

}

