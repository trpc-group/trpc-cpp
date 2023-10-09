//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/detail/writing_datagram_list.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/writing_datagram_list.h"

#include <algorithm>
#include <climits>
#include <string>
#include <utility>

namespace trpc {

ssize_t WritingDatagramList::FlushTo(Socket& socket, ConnectionHandler* conn_handler, bool* emptied) {
  std::unique_lock lk(lock_);
  if (list_.empty()) {
    *emptied = true;
    return 0;
  }

  // Pop the packet from the queue first to reduce the granularity of the lock
  auto [to, temp_io_msg] = std::move(list_.front());
  list_.pop_front();
  lk.unlock();

  thread_local iovec iov[IOV_MAX];
  std::size_t nv = 0;
  // Used for highly fragmented situations
  std::string flatten;

  if (TRPC_UNLIKELY(temp_io_msg.buffer.size() > IOV_MAX)) {  // highly fragmented
    TRPC_LOG_WARN("msg is highly fragmented and cannot be handled by `iovec`s. Flattening.");
    flatten = FlattenSlow(temp_io_msg.buffer);
    auto&& v = iov[0];
    nv = 1;
    v.iov_base = const_cast<char*>(flatten.data());
    v.iov_len = flatten.size();
  } else {
    for (auto bi = temp_io_msg.buffer.begin(); bi != temp_io_msg.buffer.end(); ++bi) {
      auto&& b = *bi;
      auto&& e = iov[nv++];
      e.iov_base = const_cast<char*>(b.data());
      e.iov_len = b.size();
    }
  }

  msghdr msg = {.msg_name = const_cast<void*>(reinterpret_cast<const void*>(to.SockAddr())),
                .msg_namelen = to.Socklen(),
                .msg_iov = iov,
                .msg_iovlen = nv,
                .msg_control = nullptr,
                .msg_controllen = 0,
                .msg_flags = 0};
  auto rc = socket.SendMsg(&msg);
  if (rc <= 0) {
    return rc;
  }

  conn_handler->MessageWriteDone(temp_io_msg);
  return rc;
}

bool WritingDatagramList::Append(NetworkAddress to, IoMessage&& io_msg) {
  std::scoped_lock lk(lock_);
  list_.emplace_back(std::tuple(std::move(to), std::move(io_msg)));
  return list_.size() > 0 ? true : false;
}

}  // namespace trpc
