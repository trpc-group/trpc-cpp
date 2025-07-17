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

#include "trpc/runtime/iomodel/async_io/async_io.h"

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

#include "liburing.h"

#include "trpc/util/object_pool/object_pool.h"

namespace trpc {

struct IOURingUserData {
  trpc::Promise<int32_t> pr;
};

namespace object_pool {

template <>
struct ObjectPoolTraits<IOURingUserData> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

struct io_uring* InitIOUring(const AsyncIO::Options& options) {
  auto ring = new struct io_uring;

  int ret = io_uring_queue_init(options.entries, ring, options.flags);
  if (ret != 0) {
    fprintf(stderr, "io_uring init failed, ret:%d msg:%s\n", ret, strerror(-ret));
    abort();
  }
  return ring;
}

void DestroyIOUring(struct io_uring* ring) {
  io_uring_queue_exit(ring);
  delete ring;
}

AsyncIO::AsyncIO(const Options& options)
    : tvar_prefix_("trpc/io_uring/io_thread_" + std::to_string(options.reactor->Id())),
      ring_(InitIOUring(options), &DestroyIOUring),
      sq_length_tvar_(tvar_prefix_ + "/sq_length", std::bind(&AsyncIO::SQLength, this)),
      cq_length_tvar_(tvar_prefix_ + "/cq_length", std::bind(&AsyncIO::CQLength, this)),
      submitted_tvar_(tvar_prefix_ + "/submitted", std::bind(&AsyncIO::Submitted, this)),
      sq_dropped_tvar_(tvar_prefix_ + "/sq_dropped", std::bind(&AsyncIO::SQDropped, this)),
      cq_overflow_tvar_(tvar_prefix_ + "/cq_overflow", std::bind(&AsyncIO::CQOverflow, this)) {
  notifier_ = std::make_unique<EventFdNotifier>(options.reactor);
  auto wakeup_func = std::bind(std::mem_fn(&AsyncIO::Poll), this);
  notifier_->SetWakeupFunction(wakeup_func);

  int ret = io_uring_register_eventfd(ring_.get(), notifier_->GetFd());
  if (ret != 0) {
    fprintf(stderr, "io_uring register eventfd failed, ret:%d msg:%s\n", ret, strerror(-ret));
    abort();
  }

  submitted_ = 0;
}

AsyncIO::~AsyncIO() {
  notifier_.reset();
}

template <typename F>
trpc::Future<int32_t> AsyncIO::SubmitOne(F&& fill_sqe) {
  struct io_uring* ring = ring_.get();
  if (*ring->cq.koverflow > 0) {
    TRPC_FMT_INFO_IF(TRPC_EVERY_N(1000), "io_uring cq is overflow, overflow:{}", *ring->cq.koverflow);
  }
  if (TRPC_UNLIKELY(submitted_ >= *(ring->cq.kring_entries))) {
    // There is a risk of CQ being full, take the initiative to obtain a result to avoid the queue being full
    // TODO: In the high-version kernel, EBUSY should be returned when submitting to determine whether the CQ is full,
    // which will be more accurate
    TRPC_FMT_INFO_IF(TRPC_EVERY_N(1000), "io_uring cq is nearly overflow, submitted:{}", submitted_);
    Poll();
    if (submitted_ >= *(ring->cq.kring_entries)) {
      return MakeExceptionFuture<int32_t>(AsyncIOError(AsyncIOError::TOO_MANY_WAIT, "Too many request is waiting"));
    }
  }

  struct io_uring_sqe* sqe = io_uring_get_sqe(ring);
  if (TRPC_UNLIKELY(sqe == NULL)) {
    return MakeExceptionFuture<int32_t>(AsyncIOError(AsyncIOError::SQ_FULL, "Send queue is full"));
  }

  fill_sqe(sqe);
  IOURingUserData* user_data = trpc::object_pool::New<IOURingUserData>();
  io_uring_sqe_set_data(sqe, user_data);

  int ret = io_uring_submit(ring);
  if (TRPC_UNLIKELY(ret <= 0)) {
    trpc::object_pool::Delete<IOURingUserData>(user_data);
    if (ret < 0) {
      return MakeExceptionFuture<int32_t>(AsyncIOError(ret));
    }
    std::string msg = "Submit success but return not one, ret:" + std::to_string(ret);
    return MakeExceptionFuture<int32_t>(AsyncIOError(AsyncIOError::SUBMIT_FAIL, msg));
  }
  submitted_ += ret;

  return user_data->pr.GetFuture();
}

trpc::Future<int32_t> AsyncIO::AsyncReadv(int fd, const struct iovec* bufs, int iovcnt, off_t offset, int flags) {
  return SubmitOne([&](struct io_uring_sqe* sqe) {
    io_uring_prep_readv(sqe, fd, bufs, iovcnt, offset);
    sqe->rw_flags = flags;
  });
}

trpc::Future<int32_t> AsyncIO::AsyncReadv(int fd, const trpc::NoncontiguousBuffer& bufs, off_t offset, int flags) {
  std::unique_ptr<struct iovec[]> iovs = ConstructIoVec(bufs);

  return AsyncReadv(fd, iovs.get(), bufs.size(), offset, flags)
      .Then([iovs = std::move(iovs)](trpc::Future<int32_t> fut) { return std::move(fut); });
}

trpc::Future<int32_t> AsyncIO::AsyncWritev(int fd, const struct iovec* bufs, int iovcnt, off_t offset, int flags) {
  return SubmitOne([&](struct io_uring_sqe* sqe) {
    io_uring_prep_writev(sqe, fd, bufs, iovcnt, offset);
    sqe->rw_flags = flags;
  });
}

trpc::Future<int32_t> AsyncIO::AsyncWritev(int fd, const trpc::NoncontiguousBuffer& bufs, off_t offset, int flags) {
  std::unique_ptr<struct iovec[]> iovs = ConstructIoVec(bufs);

  return AsyncWritev(fd, iovs.get(), bufs.size(), offset, flags)
      .Then([iovs = std::move(iovs)](trpc::Future<int32_t> fut) { return std::move(fut); });
}

std::unique_ptr<struct iovec[]> AsyncIO::ConstructIoVec(const trpc::NoncontiguousBuffer& bufs) {
  std::unique_ptr<struct iovec[]> iovs = std::make_unique<struct iovec[]>(bufs.size());
  size_t iovcnt = 0;
  for (auto& buf : bufs) {
      struct iovec* iov = iovs.get() + iovcnt;
      iov->iov_base = buf.data();
      iov->iov_len = buf.size();
      iovcnt++;
  }
  TRPC_ASSERT(iovcnt == bufs.size());

  return iovs;
}

trpc::Future<int32_t> AsyncIO::AsyncFSync(int fd) {
  return SubmitOne([&](struct io_uring_sqe* sqe) { io_uring_prep_fsync(sqe, fd, 0); });
}

  void AsyncIO::Poll() {
    struct io_uring_cqe* cqe = NULL;
    for (;;) {
      int ret = io_uring_peek_cqe(ring_.get(), &cqe);
      if (TRPC_UNLIKELY(ret != 0)) {
        if (ret != -EAGAIN) {
          TRPC_PRT_ERROR("AsyncIO::Poll peek cqe failed, ret:%d", ret);
        }
        break;
      }
      if (TRPC_LIKELY(cqe)) {
        IOURingUserData* user_data = reinterpret_cast<trpc::IOURingUserData*>(cqe->user_data);
        user_data->pr.SetValue(cqe->res);
        trpc::object_pool::Delete<IOURingUserData>(user_data);
        submitted_--;
      }
      io_uring_cqe_seen(ring_.get(), cqe);
    }
  }

  int AsyncIO::ForceSubmit() { return io_uring_submit(ring_.get()); }

  unsigned AsyncIO::SQLength() {
    // avoid nullptr before io_uring contruction
    if (!ring_) {
      return 0;
    }
    return io_uring_sq_ready(ring_.get());
  }

  unsigned AsyncIO::CQLength() {
    // avoid nullptr before io_uring contruction
    if (!ring_) {
      return 0;
    }
    return io_uring_cq_ready(ring_.get());
  }

  unsigned AsyncIO::SQDropped() {
    // avoid nullptr before io_uring contruction
    if (!ring_) {
      return 0;
    }
    return IO_URING_READ_ONCE(*ring_->sq.kdropped);
  }

  unsigned AsyncIO::CQOverflow() {
    // avoid nullptr before io_uring contruction
    if (!ring_) {
      return 0;
    }
    return IO_URING_READ_ONCE(*ring_->cq.koverflow);
  }

}  // namespace trpc

#endif  // ifndef TRPC_BUILD_INCLUDE_ASYNC_IO
