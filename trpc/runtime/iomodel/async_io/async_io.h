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

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

#include <sys/uio.h>

#include <cassert>
#include <cstring>

#include "trpc/common/future/future.h"
#include "trpc/runtime/iomodel/reactor/common/eventfd_notifier.h"
#include "trpc/tvar/tvar.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

struct io_uring;

namespace trpc {

/// @private
namespace testing {
class AsyncIOTest;
}  // namespace testing

/// @brief Asynchronous IO implementation based on io_uring
/// This type of interface is not thread-safe, and currently only supports running in merge mode.
/// The instance of AsyncIO needs to be obtained through the `GetThreadLocalReactor()->GetAsyncIO()`.
class AsyncIO final {
 public:
  struct Options {
    // reactor
    Reactor* reactor;

    // parameter for io_uring_queue_init
    unsigned entries;
    unsigned flags;
  };

  /// @brief Exception info of async Read/Write interfaces.
  ///        system error: ret_code_ < 0; user error: ret_code_ > 0
  class AsyncIOError : public ExceptionBase {
   public:
    enum UserErr {
      SQ_FULL = 1,
      SUBMIT_FAIL = 2,
      TOO_MANY_WAIT = 3,
    };

    // system error
    explicit AsyncIOError(int err) {
      ret_code_ = err;
      msg_ = strerror(err);
    }
    // user error
    AsyncIOError(int ret, const std::string& msg) {
      ret_code_ = ret;
      msg_ = msg;
    }

    const char* what() const noexcept override { return msg_.c_str(); }
  };

  explicit AsyncIO(const Options& options);

  ~AsyncIO();

  /// @private
  void EnableNotify() { notifier_->EnableNotify(); }

  /// @private
  void DisableNotify() { notifier_->DisableNotify(); }

  /// @brief Asynchronous read interface, the usage is the same as `preadv2`
  /// not supported offset = -1
  trpc::Future<int32_t> AsyncReadv(int fd, const struct iovec* bufs, int iovcnt, off_t offset, int flags = 0);

  /// @brief Asynchronous read interface, the usage is the same as `preadv2`, compatible with `NoncontiguousBuffer`
  /// not supported offset = -1
  trpc::Future<int32_t> AsyncReadv(int fd, const trpc::NoncontiguousBuffer& bufs, off_t offset, int flags = 0);

  /// @brief Asynchronous write interface, the usage is the same as `pwritev2`
  /// not supported offset = -1
  trpc::Future<int32_t> AsyncWritev(int fd, const struct iovec* bufs, int iovcnt, off_t offset, int flags = 0);

  /// @brief Asynchronous write interface, the usage is the same as `pwritev2`, compatible with `NoncontiguousBuffer`
  /// not supported offset = -1
  trpc::Future<int32_t> AsyncWritev(int fd, const trpc::NoncontiguousBuffer& bufs, off_t offset, int flags = 0);

  /// @brief File synchronization interface, the usage is the same as `fsync`
  trpc::Future<int32_t> AsyncFSync(int fd);

  /// @brief Framework use. Actively pull results
  /// @private
  void Poll();

  /// @brief Framework use. Active submission (forced to wake up the kernel)
  /// @private
  int ForceSubmit();

  /// @brief Framework use. Get the length of the io_uring submit queue,
  ///        including sqe not submitted to the kernel and submitted but not consumed
  /// @private
  unsigned SQLength();

  /// @brief Framework use. Get the length of the io_uring complete queue,
  ///        the cqe that the kernel has completed but has not been consumed by tRPC
  /// @private
  unsigned CQLength();

  /// @brief Framework use. Get the number of discarded requests in the io_uring submit queue
  /// @private
  unsigned SQDropped();

  /// @brief Framework use. Get the number of discarded requests in the io_uring complete queue
  /// @private
  unsigned CQOverflow();

  /// @brief Framework use. Get submit requests
  /// @private
  unsigned Submitted() { return submitted_; }

 private:
  template <typename F>
  [[gnu::always_inline]] trpc::Future<int32_t> SubmitOne(F&& fill_sqe);
  std::unique_ptr<struct iovec[]> ConstructIoVec(const trpc::NoncontiguousBuffer& bufs);

 private:
  std::string tvar_prefix_;
  std::unique_ptr<struct io_uring, void(*)(struct io_uring*)> ring_;
  tvar::PassiveStatus<unsigned> sq_length_tvar_;
  tvar::PassiveStatus<unsigned> cq_length_tvar_;
  tvar::PassiveStatus<unsigned> submitted_tvar_;
  tvar::PassiveStatus<unsigned> sq_dropped_tvar_;
  tvar::PassiveStatus<unsigned> cq_overflow_tvar_;
  std::unique_ptr<EventFdNotifier> notifier_;
  unsigned submitted_;
  friend class testing::AsyncIOTest;
};

}  // namespace trpc

#endif  // ifndef TRPC_BUILD_INCLUDE_ASYNC_IO
