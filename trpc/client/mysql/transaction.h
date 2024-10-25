#pragma once

#include "trpc/client/mysql/executor/mysql_executor.h"

namespace trpc::mysql {

class TransactionHandle : public RefCounted<TransactionHandle>{

 public:
  enum class TxState {kNotInited, kStart, kRollBacked, kCommitted, kInValid};

  explicit TransactionHandle(RefPtr<MysqlExecutor> &&executor) : executor_(std::move(executor)) {}

  TransactionHandle() :executor_(nullptr) {}

  TransactionHandle& operator=(TransactionHandle&& other) noexcept {
    state_ = other.state_;
    executor_ = std::move(other.executor_);
    other.state_ = TxState::kInValid;
    other.executor_ = nullptr;
    return *this;
  }

  TransactionHandle(TransactionHandle&& other) noexcept {
    state_ = other.state_;
    executor_ = std::move(other.executor_);
    other.state_ = TxState::kInValid;
    other.executor_ = nullptr;
  }

  TransactionHandle& operator=(const TransactionHandle& other) = delete;
  TransactionHandle(const TransactionHandle& other) = delete;



  ~TransactionHandle() {
    // usually executor_ would be nullptr
    if(executor_)
      executor_->Close();
    state_ = TxState::kInValid;
  }

  void SetState(TxState state) { state_ = state; }

  TxState GetState() { return state_; }

  bool SetExecutor(RefPtr<MysqlExecutor> &&executor) {
    if(executor_)
        return false;
    executor_ = std::move(executor);
    return true;
  }

  RefPtr<MysqlExecutor> GetExecutor() { return executor_; }

  RefPtr<MysqlExecutor>&& TransferExecutor() { return std::move(executor_); }

 private:
  RefPtr<MysqlExecutor> executor_{nullptr};
  TxState state_{TxState::kNotInited};
};


using TxHandlePtr = RefPtr<TransactionHandle>;
}

