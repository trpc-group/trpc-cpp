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

#include "trpc/util/async_io/async_io.h"

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

#include <fcntl.h>
#include <poll.h>

#include "gtest/gtest.h"
#include "liburing.h"

#include "trpc/runtime/iomodel/reactor/common/epoll_poller.h"
#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"

namespace trpc::testing {

static constexpr size_t kBufferSize = 4096;

uint64_t FillBuffer(char* ptr, size_t size, uint64_t seed = 0x7777777777777777ULL) {
  for (size_t offset = 0; offset < size; offset += sizeof(seed)) {
    *(reinterpret_cast<uint64_t*>(ptr + offset)) = seed;
    seed = seed * 5 + 1;
  }
  return seed;
}

bool CheckBuffer(char* ptr, size_t size, uint64_t seed = 0x7777777777777777ULL) {
  for (size_t offset = 0; offset < size; offset += sizeof(seed)) {
    if (*(reinterpret_cast<uint64_t*>(ptr + offset)) != seed) {
      std::cerr << "CheckBuffer failed at " << offset << std::endl;
      return false;
    }
    seed = seed * 5 + 1;
  }
  return true;
}

class AsyncIOTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ReactorImpl::Options reactor_options;
    reactor_options.id = 0;
    reactor_options.enable_async_io = true;

    reactor_ = std::make_unique<ReactorImpl>(reactor_options);
    reactor_->Initialize();

    memset(buffer_, 0, kBufferSize);

    sprintf(filename, "./test_async_io.XXXXXX");

    file_fd_ = mkostemp(filename, O_RDWR);
    ASSERT_GT(file_fd_, 0) << "mkstemp failed, " << strerror(errno);

    unlink(filename);
  }

  void TearDown() override {
    reactor_->Destroy();
  }

  io_uring* GetIOUring(const AsyncIO* async_io) {
    return async_io->ring_.get();
  }

 protected:
  std::unique_ptr<Reactor> reactor_;
  char buffer_[kBufferSize];
  char filename[64];
  int file_fd_;
};

TEST_F(AsyncIOTest, liburingTest) {
  struct io_uring ring;
  int ret = io_uring_queue_init(128, &ring, 0);
  ASSERT_EQ(ret, 0) << "io_uring_queue_init failed, ret:" << ret;
  io_uring_queue_exit(&ring);
}

TEST_F(AsyncIOTest, AsyncReadv) {
  std::thread t1([this]() { this->reactor_->Run(); });

  FillBuffer(buffer_, kBufferSize);
  ASSERT_EQ(pwrite(file_fd_, buffer_, kBufferSize, 0), kBufferSize);
  memset(buffer_, 0, kBufferSize);
  usleep(100000);

  int ret = 0;
  reactor_->SubmitTask([&ret, reactor = reactor_.get(), fd = file_fd_, buffer = buffer_]() {
    struct iovec* bufs = new struct iovec({buffer, kBufferSize});

    reactor->GetAsyncIO()
        ->AsyncReadv(fd, bufs, 1, 0)
        .Then([&ret, bufs, buffer](Future<int32_t>&& fut) {
          if (fut.IsFailed()) {
            EXPECT_FALSE(fut.IsFailed()) << fut.GetException().what();
          } else {
            int res = std::get<0>(fut.GetValue());
            if (res < 0) {
              EXPECT_GE(res, 0) << "readv failed, " << strerror(-res);
            } else {
              EXPECT_EQ(res, kBufferSize) << "read not finish";
              ret += res;
              EXPECT_TRUE(CheckBuffer(buffer, kBufferSize));
            }
          }
          delete bufs;
          return MakeReadyFuture<>();
        });
  });
  usleep(100000);

  reactor_->Stop();

  t1.join();

  EXPECT_EQ(ret, kBufferSize);
}

TEST_F(AsyncIOTest, AsyncReadvToNoncontiguousBuffer) {
  std::thread t1([this]() { this->reactor_->Run(); });

  FillBuffer(buffer_, kBufferSize);
  ASSERT_EQ(pwrite(file_fd_, buffer_, kBufferSize, 0), kBufferSize);
  usleep(100000);

  int ret = 0;
  reactor_->SubmitTask([&ret, reactor = reactor_.get(), fd = file_fd_, buffer = buffer_]() {
    NoncontiguousBufferBuilder buf_builder;
    (void)buf_builder.Reserve(2048);
    (void)buf_builder.Reserve(2048);

    auto bufs = std::make_unique<NoncontiguousBuffer>(buf_builder.DestructiveGet());
    EXPECT_EQ(bufs->size(), 2);
    EXPECT_EQ(bufs->ByteSize(), 4096);

    reactor->GetAsyncIO()
        ->AsyncReadv(fd, *bufs.get(), 0)
        .Then([&ret, buffer, bufs = std::move(bufs)](Future<int32_t>&& fut) {
          if (fut.IsFailed()) {
            EXPECT_FALSE(fut.IsFailed()) << fut.GetException().what();
          } else {
            int res = std::get<0>(fut.GetValue());
            if (res < 0) {
              EXPECT_GE(res, 0) << "readv failed, " << strerror(-res);
            } else {
              EXPECT_EQ(res, 4096) << "read not finish";
              ret += res;
              uint64_t offset = 0;
              for (auto& buf : *bufs.get()) {
                EXPECT_EQ(memcmp(buf.data(), buffer + offset, buf.size()), 0)
                    << "failed from " << offset << " to " << offset + buf.size();
                offset += buf.size();
              }
            }
          }
          return MakeReadyFuture<>();
        });
  });

  usleep(100000);

  reactor_->Stop();

  t1.join();

  EXPECT_EQ(ret, kBufferSize);
}

TEST_F(AsyncIOTest, AsyncWritev) {
  std::thread t1([this]() { this->reactor_->Run(); });

  usleep(100000);

  int ret = 0;
  reactor_->SubmitTask([&ret, reactor = reactor_.get(), fd = file_fd_, buffer = buffer_]() {
    struct iovec* bufs = new struct iovec({buffer, kBufferSize});
    FillBuffer(buffer, kBufferSize);

    reactor->GetAsyncIO()->AsyncWritev(fd, bufs, 1, 0).Then([&ret, bufs](Future<int32_t>&& fut) {
      if (fut.IsFailed()) {
        EXPECT_FALSE(fut.IsFailed()) << fut.GetException().what();
      } else {
        int res = std::get<0>(fut.GetValue());
        if (res < 0) {
          EXPECT_GE(res, 0) << "writev failed, " << strerror(-res);
        } else {
          EXPECT_EQ(res, kBufferSize) << "write not finish";
          ret += res;
        }
      }
      delete bufs;
      return MakeReadyFuture<>();
    });
  });

  usleep(100000);

  reactor_->Stop();

  t1.join();

  ASSERT_EQ(ret, kBufferSize);

  memset(buffer_, 0, kBufferSize);
  ASSERT_EQ(pread(file_fd_, buffer_, kBufferSize, 0), kBufferSize)
      << "read failed, " << strerror(errno);

  EXPECT_TRUE(CheckBuffer(buffer_, kBufferSize));
}

TEST_F(AsyncIOTest, AsyncWritevFromNoncontiguousBuffer) {
  std::thread t1([this]() { this->reactor_->Run(); });

  usleep(100000);

  int ret = 0;
  reactor_->SubmitTask([&ret, reactor = reactor_.get(), fd = file_fd_]() {
    NoncontiguousBufferBuilder buf_builder;
    char* ptr = buf_builder.Reserve(2048);
    uint64_t seed = FillBuffer(ptr, 2048);
    ptr = buf_builder.Reserve(2048);
    FillBuffer(ptr, 2048, seed);

    auto bufs = std::make_unique<NoncontiguousBuffer>(buf_builder.DestructiveGet());
    EXPECT_EQ(bufs->size(), 2);
    EXPECT_EQ(bufs->ByteSize(), 4096);

    reactor->GetAsyncIO()
        ->AsyncWritev(fd, *bufs.get(), 0)
        .Then([&ret, bufs = std::move(bufs)](Future<int32_t>&& fut) {
          if (fut.IsFailed()) {
            EXPECT_FALSE(fut.IsFailed()) << fut.GetException().what();
          } else {
            int res = std::get<0>(fut.GetValue());
            if (res < 0) {
              EXPECT_GE(res, 0) << "writev failed, " << strerror(-res);
            } else {
              EXPECT_EQ(res, 4096) << "write not finish";
              ret += res;
            }
          }
          return MakeReadyFuture<>();
        });
  });

  usleep(100000);

  reactor_->Stop();

  t1.join();

  ASSERT_EQ(ret, kBufferSize);

  memset(buffer_, 0, kBufferSize);
  ASSERT_EQ(pread(file_fd_, buffer_, kBufferSize, 0), kBufferSize)
      << "read failed, " << strerror(errno);

  EXPECT_TRUE(CheckBuffer(buffer_, kBufferSize));
}

TEST_F(AsyncIOTest, AsyncFSync) {
  std::thread t1([this]() { this->reactor_->Run(); });

  usleep(100000);

  reactor_->SubmitTask([reactor = reactor_.get(), fd = file_fd_]() {
    reactor->GetAsyncIO()->AsyncFSync(fd).Then([](Future<int32_t>&& fut) {
      if (fut.IsFailed()) {
        EXPECT_FALSE(fut.IsFailed()) << fut.GetException().what();
      } else {
        int res = std::get<0>(fut.GetValue());
        EXPECT_GE(res, 0) << "fsync failed, " << strerror(-res);
      }
      return MakeReadyFuture<>();
    });
  });

  usleep(100000);

  reactor_->Stop();

  t1.join();
}

TEST_F(AsyncIOTest, SQFull) {
  std::thread t1([this]() { this->reactor_->Run(); });

  usleep(100000);

  auto ring = GetIOUring(reactor_->GetAsyncIO());
  while (io_uring_sq_space_left(ring) > 0) {
    ASSERT_NE(io_uring_get_sqe(ring), nullptr);
  }

  int ret = 0;
  reactor_->SubmitTask([&ret, reactor = reactor_.get(), fd = file_fd_]() {
    char* data = new char('x');
    struct iovec bufs;
    bufs.iov_base = data;
    bufs.iov_len = 1;

    reactor->GetAsyncIO()->AsyncWritev(fd, &bufs, 1, 0).Then([&ret, data](Future<int32_t>&& fut) {
      if (fut.IsFailed()) {
        auto exp = fut.GetException();
        EXPECT_TRUE(exp.is<AsyncIO::AsyncIOError>());
        EXPECT_EQ(exp.GetExceptionCode(), AsyncIO::AsyncIOError::SQ_FULL);
      } else {
        EXPECT_TRUE(fut.IsFailed());
      }
      delete data;
      return MakeReadyFuture<>();
    });
  });

  usleep(100000);

  EXPECT_EQ(ret, 0);

  reactor_->Stop();

  t1.join();

  EXPECT_EQ(ret, 0);

  EXPECT_EQ(0, io_uring_sq_space_left(ring));
  EXPECT_EQ(1024, reactor_->GetAsyncIO()->ForceSubmit());
  EXPECT_EQ(1024, io_uring_sq_space_left(ring));
}

TEST_F(AsyncIOTest, SubmitFailed) {
  std::thread t1([this]() { this->reactor_->Run(); });

  usleep(100000);

  auto ring = GetIOUring(reactor_->GetAsyncIO());
  int save_fd = ring->ring_fd;
  ring->ring_fd = -1;

  reactor_->SubmitTask([reactor = reactor_.get()]() {
    char* data = new char;
    *data = 'x';
    struct iovec bufs;
    bufs.iov_base = data;
    bufs.iov_len = 1;

    reactor->GetAsyncIO()->AsyncReadv(2333, &bufs, 1, 0).Then([data](Future<int32_t>&& fut) {
      if (fut.IsFailed()) {
        auto exp = fut.GetException();
        EXPECT_TRUE(exp.is<AsyncIO::AsyncIOError>());
        EXPECT_LT(exp.GetExceptionCode(), 0);
      } else {
        EXPECT_TRUE(fut.IsFailed()) << "ret:" << std::get<0>(fut.GetValue());
      }
      delete data;
      return MakeReadyFuture<>();
    });
  });

  usleep(100000);

  reactor_->Stop();

  t1.join();

  ring->ring_fd = save_fd;
}

}  // namespace trpc::testing

#endif  // ifndef TRPC_BUILD_INCLUDE_ASYNC_IO
