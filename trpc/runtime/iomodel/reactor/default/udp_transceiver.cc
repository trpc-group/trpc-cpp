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

#include "trpc/runtime/iomodel/reactor/default/udp_transceiver.h"

#include <errno.h>
#include <string.h>

#include <functional>
#include <memory>
#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc {

UdpTransceiver::UdpTransceiver(Reactor* reactor, const Socket& socket, bool is_client)
    : reactor_(reactor), socket_(socket) {
  TRPC_ASSERT(reactor_);
  TRPC_ASSERT(socket_.IsValid());

  if (!is_client) {
    socket_.SetReuseAddr();
    socket_.SetReusePort();
  }
  socket_.SetBlock(false);
  SetFd(socket_.GetFd());
}

UdpTransceiver::~UdpTransceiver() {
  if (GetInitializationState() != InitializationState::kInitializedSuccess) {
    return;
  }

  TRPC_ASSERT(!socket_.IsValid());
  TRPC_ASSERT(!enable_);
}

void UdpTransceiver::StartHandshaking() { TRPC_ASSERT(GetIoHandler()); }

int UdpTransceiver::Send(IoMessage&& msg) {
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

  io_msgs_.emplace_back(std::move(msg));

  HandleWriteEvent();

  return 0;
}

void UdpTransceiver::DoClose(bool destroy) { HandleClose(destroy); }

void UdpTransceiver::EnableReadWrite() {
  if (enable_) {
    return;
  }

  int events = EventHandler::EventType::kReadEvent | EventHandler::EventType::kWriteEvent;

  EnableEvent(events);

  Ref();

  reactor_->Update(this);

  enable_ = true;

  SetInitializationState(InitializationState::kInitializedSuccess);
}

void UdpTransceiver::DisableReadWrite() {
  if (!enable_) {
    return;
  }

  read_buffer_.Clear();

  enable_ = false;

  DisableAllEvent();

  reactor_->Update(this);

  Deref();

  socket_.Close();
}

int UdpTransceiver::HandleReadEvent() {
  read_buffer_.Clear();
  while (true) {
    int n = ReadIoData(read_buffer_);
    if (n > 0) {
      std::deque<std::any> data;
      RefPtr ref(ref_ptr, this);
      int ret = GetConnectionHandler()->CheckMessage(ref, read_buffer_, data);
      if (ret == kPacketFull) {
        GetConnectionHandler()->HandleMessage(ref, data);
      } else if (ret == kPacketError) {
        // only discard the packet received, no need to close the socket
        break;
      }
    } else {
      break;
    }
  }

  return 0;
}

int UdpTransceiver::HandleWriteEvent() {
  int send_finish = 0;

  for (const auto& msg : io_msgs_) {
    BufferPtr buff = trpc::MakeRefCounted<Buffer>(msg.buffer.ByteSize());
    FlattenToSlow(msg.buffer, buff->GetWritePtr(), buff->WritableSize());
    buff->AddWriteLen(buff->WritableSize());

    NetworkAddress peer_addr(msg.ip, msg.port, NetworkAddress::IpType::kUnknown);
    int n = socket_.SendTo(buff->GetReadPtr(), buff->ReadableSize(), 0, peer_addr);
    if (n < 0) {
      TRPC_FMT_ERROR("Send error, reason = {}, peer addr = {}", strerror(errno), peer_addr.ToString());
      // only need to retry sending the packet in this case to avoid continuous increase of the queue
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        break;
      }
      break;
    }

    send_finish++;
  }

  while (send_finish > 0) {
    IoMessage& temp = io_msgs_.front();

    MessageWriteDone(temp);

    io_msgs_.pop_front();

    --send_finish;
  }

  return 0;
}

void UdpTransceiver::HandleCloseEvent() {
  TRPC_LOG_ERROR("UdpTransceiver::HandleCloseEvent fd:" << socket_.GetFd() << ", is_client:" << IsClient()
                                                        << ", has epoll event error.");
}

void UdpTransceiver::HandleClose(bool destroy) {
  if (!enable_) {
    return;
  }

  TRPC_LOG_ERROR("UdpTransceiver::HandleClose fd:" << socket_.GetFd() << ", conn_id:" << this->GetConnId()
                                                   << ", is_client:" << IsClient());

  for (auto& msg : io_msgs_) {
    GetConnectionHandler()->NotifyMessageFailed(msg, ConnectionErrorCode::kNetworkException);
  }
  io_msgs_.clear();

  DisableReadWrite();

  GetConnectionHandler()->ConnectionClosed();
  if (destroy) {
    GetConnectionHandler()->CleanResource();
  }
}

void UdpTransceiver::MessageWriteDone(IoMessage& msg) { GetConnectionHandler()->MessageWriteDone(msg); }

int UdpTransceiver::ReadIoData(NoncontiguousBuffer& buff) {
  NetworkAddress peer_addr;
  int n = socket_.RecvFrom(static_cast<void*>(recv_buffer_), kUdpBuffSize, 0, &peer_addr);
  if (n > 0) {
    buff.Append(CreateBufferSlow(recv_buffer_, n));

    SetPeerIp(peer_addr.Ip());
    SetPeerPort(peer_addr.Port());
    return n;
  } else if (n < 0) {
    if (errno == EAGAIN) {
      return n;
    }
    TRPC_LOG_ERROR("UdpTransceiver::ReadIoData read datagram error, fd:"
                   << socket_.GetFd() << ", conn_id:" << this->GetConnId() << ", is_client:" << IsClient()
                   << ", errno:" << errno);
    // It can continue to read even if failed to read a udp packet
  }
  return n;
}

}  // namespace trpc
