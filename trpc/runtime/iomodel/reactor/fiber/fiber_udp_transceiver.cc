//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/native/datagram_transceiver.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/fiber_udp_transceiver.h"

#include <limits>
#include <utility>

namespace trpc {

FiberUdpTransceiver::FiberUdpTransceiver(Reactor* reactor, const Socket& socket, bool is_client)
    : FiberConnection(reactor), socket_(socket) {
  TRPC_ASSERT(socket_.IsValid());
  if (!is_client) {
    socket_.SetReuseAddr();
    socket_.SetReusePort();
  }
  socket_.SetBlock(false);
  SetFd(socket_.GetFd());
}

FiberUdpTransceiver::~FiberUdpTransceiver() {
  TRPC_LOG_DEBUG("~FiberUdpTransceiver fd:" << socket_.GetFd() << ", conn_id:" << this->GetConnId());
  // Validity checks should only be performed when the initialization is successful
  if (GetInitializationState() != InitializationState::kInitializedSuccess) {
    return;
  }

  TRPC_ASSERT(!socket_.IsValid());
}

void FiberUdpTransceiver::EnableReadWrite() {
  EnableEvent(EventHandler::EventType::kReadEvent);

  AttachReactor();

  // Set to initialization success state
  SetInitializationState(InitializationState::kInitializedSuccess);
}

void FiberUdpTransceiver::RestartWriteEvent() {
  // The FlushWritingBuffer function of udp can be executed concurrently, and RestartWriteEvent may be called multiple
  // times. Here, we limit the registration of the write event to only one time by using the restart_write_count_
  std::size_t expected = 0;
  if (restart_write_count_.compare_exchange_strong(expected, 1, std::memory_order_relaxed)) {
    GetReactor()->SubmitTask([this, ref = RefPtr(ref_ptr, this)] {
      if (Enabled()) {
        TRPC_ASSERT((GetSetEvents() & EventType::kWriteEvent) == 0);
        SetSetEvents(GetSetEvents() | EventType::kWriteEvent);
        GetReactor()->Update(this);
      }
    });
  }
}

int FiberUdpTransceiver::Send(IoMessage&& msg) {
  // If the packet to be sent is empty, return an error directly
  if (msg.buffer.ByteSize() == 0) {
    TRPC_FMT_ERROR("Empty msg, dst ip:{}, port:{}", msg.ip, msg.port);
    return -1;
  }

  // If the packet to be sent is larger than the maximum udp length, return an error directly
  if (msg.buffer.ByteSize() > kMaxUdpBodySize) {
    TRPC_FMT_ERROR("Msg too long, size = {}, dst ip:{}, port:{}", msg.buffer.ByteSize(), msg.ip, msg.port);
    return -1;
  }

  return SendWithDatagramList(std::move(msg));
}

int FiberUdpTransceiver::SendWithDatagramList(IoMessage&& msg) {
  NetworkAddress to(msg.ip, msg.port, NetworkAddress::IpType::kUnknown);
  if (write_list_.Append(std::move(to), std::move(msg))) {
    auto rc = FlushWritingBuffer(max_writes_percall_);

    if (rc == FlushStatus::kSystemBufferSaturated || rc == FlushStatus::kQuotaExceeded) {
      RestartWriteEvent();
    } else if (rc == FlushStatus::kFlushed) {
    } else if (rc == FlushStatus::kPartialWrite || rc == FlushStatus::kError) {
      TRPC_LOG_ERROR("FiberUdpTransceiver::Send failed is_client:"
                     << IsClient() << ", ip:" << msg.ip << ", port:" << msg.port << ", fd:" << socket_.GetFd()
                     << ", rc:" << static_cast<int>(rc) << ", errno:" << errno);
      return -1;
    } else if (rc == FlushStatus::kNothingWritten) {
      TRPC_LOG_ERROR("FiberUdpTransceiver::Send failed is_client:"
                     << IsClient() << ", ip:" << msg.ip << ", port:" << msg.port << ", fd:" << socket_.GetFd()
                     << ", rc:" << static_cast<int>(rc) << ", errno:" << errno);
      return -1;
    } else {
      TRPC_ASSERT(false);
    }
    return 0;
  }

  TRPC_LOG_ERROR("FiberUdpTransceiver::Send failed to write ip:" << msg.ip << ", port:" << msg.port << ", is_client:"
                                                                 << IsClient() << ", append buffer failed.");
  return -1;
}

void FiberUdpTransceiver::Stop() {
  // Resource cleanup should only be performed when the initialization is successful
  if (GetInitializationState() == InitializationState::kInitializedSuccess) {
    Kill(CleanupReason::UserInitiated);
  }
}

void FiberUdpTransceiver::Join() {
  // Resource cleanup should only be performed when the initialization is successful
  if (GetInitializationState() == InitializationState::kInitializedSuccess) {
    WaitForCleanup();
  }
}

FiberConnection::EventAction FiberUdpTransceiver::OnReadable() {
  read_buffer_.Clear();

  while (true) {
    thread_local char recv_buffer[kUdpBuffSize] = {0};

    NetworkAddress peer_addr;
    int read = socket_.RecvFrom(static_cast<void*>(recv_buffer), kUdpBuffSize, 0, &peer_addr);
    if (read < 0) {
      if (errno == EAGAIN) {
        break;
      } else {
        TRPC_LOG_ERROR("FiberUdpTransceiver::OnReadable read datagram error, fd:"
                       << socket_.GetFd() << ", conn_id:" << this->GetConnId() << ", is_client:" << IsClient()
                       << ", ip:" << GetPeerIp() << ", port:" << GetPeerPort() << ", errno:" << errno);
        // only discard the packet received, no need to close the socket
        return EventAction::kReady;
      }
    }

    SetPeerIp(peer_addr.Ip());
    SetPeerPort(peer_addr.Port());
    TRPC_ASSERT(static_cast<uint32_t>(read) <= kUdpBuffSize);
    read_buffer_.Append(CreateBufferSlow(recv_buffer, read));

    RefPtr ref(ref_ptr, this);
    std::deque<std::any> data;
    int checker_ret = GetConnectionHandler()->CheckMessage(ref, read_buffer_, data);
    if (checker_ret == kPacketFull) {
      bool handle_ret = GetConnectionHandler()->HandleMessage(ref, data);
      if (!handle_ret) {
        TRPC_LOG_ERROR("FiberUdpTransceiver::OnReadable MessageHandle error, fd:"
                       << socket_.GetFd() << ", conn_id:" << this->GetConnId() << ", is_client:" << IsClient()
                       << ", ip:" << GetPeerIp() << ", port:" << GetPeerPort());
        break;
      }
    } else if (checker_ret == kPacketError) {
      TRPC_LOG_ERROR("FiberUdpTransceiver::OnReadable check error, fd:"
                     << socket_.GetFd() << ", conn_id:" << this->GetConnId() << ", is_client:" << IsClient()
                     << ", ip:" << GetPeerIp() << ", port:" << GetPeerPort());
      // only discard the packet received, no need to close the socket
      return EventAction::kReady;
    }
  }
  return EventAction::kReady;
}

FiberConnection::EventAction FiberUdpTransceiver::OnWritable() {
  auto status = FlushWritingBuffer(std::numeric_limits<std::size_t>::max());

  if (status == FlushStatus::kSystemBufferSaturated) {
    return EventAction::kReady;
  } else if (status == FlushStatus::kFlushed) {
    return EventAction::kSuppress;
  } else if (status == FlushStatus::kPartialWrite || status == FlushStatus::kNothingWritten ||
             status == FlushStatus::kError) {
    TRPC_LOG_ERROR("FiberUdpTransceiver::OnWritable send datagram error, fd:"
                   << socket_.GetFd() << ", conn_id:" << this->GetConnId() << ", is_client:" << IsClient()
                   << ", ip:" << GetPeerIp() << ", port:" << GetPeerPort() << ", status:" << static_cast<int>(status));
    return EventAction::kReady;
  } else {
    TRPC_ASSERT(false);
  }
}

void FiberUdpTransceiver::OnError(int err) {
  TRPC_LOG_ERROR("FiberUdpTransceiver::OnError fd:" << socket_.GetFd() << ", conn_id:" << this->GetConnId()
                                                    << ", is_client:" << IsClient() << ", ip:" << GetPeerIp()
                                                    << ", port:" << GetPeerPort() << ", err:" << err);
}

void FiberUdpTransceiver::OnCleanup(CleanupReason reason) {
  TRPC_ASSERT(reason != CleanupReason::kNone);

  TRPC_LOG_DEBUG("FiberUdpTransceiver::OnCleanup fd:" << socket_.GetFd() << ", conn_id:" << this->GetConnId()
                                                      << ", is_client:" << IsClient()
                                                      << ", reason:" << static_cast<int>(reason));
  if (IsClient()) {
    GetConnectionHandler()->CleanResource();
  }

  socket_.Close();
}

FiberUdpTransceiver::FlushStatus FiberUdpTransceiver::FlushWritingBuffer(std::size_t max_writes) {
  while (max_writes--) {
    bool emptied;

    auto rc = write_list_.FlushTo(socket_, GetConnectionHandler(), &emptied);
    if (rc == 0) {
      return emptied ? FlushStatus::kFlushed : FlushStatus::kNothingWritten;
    } else if (rc < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        return FlushStatus::kSystemBufferSaturated;
      } else {
        return FlushStatus::kError;
      }
    }
  }
  return FlushStatus::kQuotaExceeded;
}

}  // namespace trpc
