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

#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <functional>
#include <iostream>
#include <memory>
#include <utility>

#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

TcpConnection::TcpConnection(Reactor* reactor, const Socket& socket) : reactor_(reactor), socket_(socket) {
  TRPC_ASSERT(reactor_);
  TRPC_ASSERT(socket_.IsValid());

  SetFd(socket_.GetFd());
}

TcpConnection::~TcpConnection() {
  TRPC_ASSERT(GetConnectionState() == ConnectionState::kUnconnected);
  TRPC_ASSERT(!enable_);
}

void TcpConnection::Established() {
  SetEstablishTimestamp(trpc::time::GetMilliSeconds());
  SetConnectionState(ConnectionState::kConnected);
  SetConnActiveTime(trpc::time::GetMilliSeconds());

  TRPC_ASSERT(GetIoHandler());
  TRPC_ASSERT(GetConnectionHandler());

  GetConnectionHandler()->ConnectionEstablished();

  EnableReadWrite();

  TRPC_LOG_TRACE("TcpConnection::Established fd:" << socket_.GetFd() << ", ip:" << GetPeerIp() << ", port:"
                                                  << GetPeerPort() << ", is_client:" << IsClient() << ", Connect ok.");
}

bool TcpConnection::DoConnect() {
  SetDoConnectTimestamp(trpc::time::GetMilliSeconds());

  TRPC_ASSERT(GetIoHandler());
  TRPC_ASSERT(GetConnectionHandler());

  NetworkAddress remote_addr(GetPeerIp(), GetPeerPort(), GetPeerIpType());

  int ret = socket_.Connect(remote_addr);
  if (ret == 0) {
    handshake_status_ = IoHandler::HandshakeStatus::kFailed;
    TRPC_LOG_TRACE("TcpConnection::DoConnect target: " << remote_addr.ToString() << ", fd: " << socket_.GetFd()
                                                       << ", bgein.");

    if (errno == EINPROGRESS) {
      SetConnectionState(ConnectionState::kConnecting);
      SetConnActiveTime(trpc::time::GetMilliSeconds());
      EnableReadWrite();

      if (GetCheckConnectTimeout() > 0) {
        uint64_t timer_id = reactor_->AddTimerAfter(GetCheckConnectTimeout(), 0, [ref = RefPtr(ref_ptr, this), this]() {
          // if connection status is still kConnecting,
          // notify request failed and connection timeout callback, then close the connection
          if (ref->GetConnectionState() == ConnectionState::kConnecting) {
            for (auto& msg : io_msgs_) {
              GetConnectionHandler()->NotifyMessageFailed(msg, ConnectionErrorCode::kConnectingTimeout);
            }
            ref->GetConnectionHandler()->ConnectionTimeout();
            ref->DoClose(true);
          }
        });
        reactor_->DetachTimer(timer_id);
      }
    } else {
      Established();
    }

    return true;
  }

  TRPC_LOG_ERROR("TcpConnection::DoConnect target: " << remote_addr.ToString() << ", fd: " << socket_.GetFd()
                                                     << ", failed. error no: " << errno
                                                     << ", msg: " << strerror(errno));

  GetIoHandler()->Destroy();
  socket_.Close();
  HandleClose(false);

  return false;
}

void TcpConnection::DoClose(bool destroy) { HandleClose(destroy); }

void TcpConnection::StartHandshaking() { Handshaking(true); }

void TcpConnection::Handshaking(bool is_read_event) {
  if (handshake_status_ != IoHandler::HandshakeStatus::kSucc && GetIoHandler()) {
    handshake_status_ = GetIoHandler()->Handshake(is_read_event);

    if (handshake_status_ == IoHandler::HandshakeStatus::kFailed) {
      TRPC_LOG_ERROR("TcpConnection::HandleReadEvent fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                          << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                          << ", handshake failed.");
      HandleClose(true);
    }
  }
}

void TcpConnection::EnableReadWrite() {
  if (enable_) {
    return;
  }

  int events = EventHandler::EventType::kReadEvent | EventHandler::EventType::kWriteEvent;

  EnableEvent(events);

  Ref();

  reactor_->Update(this);

  enable_ = true;
}

void TcpConnection::DisableReadWrite() {
  if (!enable_) {
    return;
  }

  enable_ = false;

  DisableAllEvent();

  reactor_->Update(this);

  Deref();

  socket_.Close();
}

void TcpConnection::DisableRead() {
  if (!enable_) {
    return;
  }

  DisableEvent(EventHandler::EventType::kReadEvent);
  reactor_->Update(this);
  TRPC_FMT_DEBUG("TcpConnection::DisableRead ip {}, port: {}, is_client {}, Disable Read.", GetPeerIp(), GetPeerPort(),
                 IsClient());
}

int TcpConnection::HandleReadEvent() {
  if (GetConnectionState() == ConnectionState::kUnconnected) {
    return -1;
  }

  Handshaking(true);
  if (handshake_status_ == IoHandler::HandshakeStatus::kFailed) {
    return -1;
  }

  int ret = ReadIoData(read_buffer_.buffer);
  if (ret > 0) {
    std::deque<std::any> data;
    RefPtr ref(ref_ptr, this);
    int checker_ret = GetConnectionHandler()->CheckMessage(ref, read_buffer_.buffer, data);
    if (checker_ret == kPacketFull) {
      bool flag = GetConnectionHandler()->HandleMessage(ref, data);
      if (TRPC_UNLIKELY(!flag)) {
        TRPC_LOG_ERROR("TcpConnection::HandleReadEvent fd:" << socket_.GetFd() << ", ip:" << GetPeerIp() << ", port:"
                                                            << GetPeerPort() << ", is_client:" << IsClient()
                                                            << ", message handle failed and connection close.");
        HandleClose(true);
        return -1;
      } else {
        // because the response packet may be returned in the MessageHandle,
        // if return failed, the connection will be closed, so here need to return directly
        if (TRPC_UNLIKELY(GetConnectionState() == ConnectionState::kUnconnected)) {
          return -1;
        }

        SetConnActiveTime(trpc::time::GetMilliSeconds());
        GetConnectionHandler()->UpdateConnection();
      }
    } else if (checker_ret == kPacketError) {
      TRPC_LOG_ERROR("TcpConnection::HandleReadEvent fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                          << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                          << ", checker packet failed and connection close.");
      HandleClose(true);
      return -1;
    }

    return 0;
  } else if (ret == 0) {
    return 0;
  }

  return -1;
}

int TcpConnection::HandleWriteEvent() {
  if (GetConnectionState() == ConnectionState::kUnconnected) {
    return -1;
  }

  if (!PreCheckOnWrite()) {
    if (handshake_status_ != IoHandler::HandshakeStatus::kFailed) {
      return 0;
    }
    return -1;
  }

  if (io_msgs_.empty()) {
    return 0;
  }

  int ret = 0;
  int send_msgs_size = 0;
  struct iovec iov[kSendDataMergeNum];

  // get how many DataBlocks there are
  for (auto& msg : io_msgs_) {
    const auto& buffer = msg.buffer;
    send_msgs_size += buffer.size();
  }

  // the index of DataBlocks has been sent
  int index = 0;
  while (index < send_msgs_size) {
    int last_index = index;
    int iov_index = 0;
    // record the length of data to be sent each time
    uint32_t total_size = 0;
    bool flag = true;
    for (auto& msg : io_msgs_) {
      const auto& buf = msg.buffer;

      for (auto iter = buf.begin(); iter != buf.end() && iov_index < kSendDataMergeNum; ++iter) {
        iov[iov_index].iov_base = iter->data();
        iov[iov_index].iov_len = iter->size();
        total_size += iter->size();

        ++index;
        ++iov_index;
      }

      if (iov_index >= kSendDataMergeNum) {
        break;
      }
    }

    int n = GetIoHandler()->Writev(iov, iov_index);
    if (n < 0) {
      if (errno != EAGAIN) {
        if (errno == EINTR) {
          index = last_index;
          continue;
        }
        ret = -1;
        TRPC_LOG_ERROR("TcpConnection::HandleWriteEvent fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                             << ", port:" << GetPeerPort()
                                                             << ", is_client:" << IsClient() << ", errno:" << errno
                                                             << ", write failed and connection close.");
      }
      need_direct_write_ = false;
      flag = false;
    } else {
      send_data_size_ -= n;
      if (static_cast<uint32_t>(n) < total_size) {
        need_direct_write_ = false;
        flag = false;
      }

      while (n > 0) {
        IoMessage& temp = io_msgs_.front();
        auto& buff = temp.buffer;

        if (static_cast<uint32_t>(n) >= buff.ByteSize()) {
          n -= buff.ByteSize();

          MessageWriteDone(temp);

          io_msgs_.pop_front();
        } else {
          buff.Skip(n);
          break;
        }
      }

      total_size = 0;
    }

    if (!flag) {
      break;
    }
  }

  if (ret == 0) {
    SetConnActiveTime(trpc::time::GetMilliSeconds());
    GetConnectionHandler()->UpdateConnection();
  } else {
    HandleClose(true);
    return -1;
  }

  return 0;
}

void TcpConnection::HandleCloseEvent() {
  if (GetConnectionState() == ConnectionState::kUnconnected) {
    return;
  }

  TRPC_LOG_DEBUG("TcpConnection::HandleCloseEvent fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                       << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                       << ", epoll event error and connection close.");

  HandleClose(true);
}

void TcpConnection::HandleClose(bool destroy) {
  if (GetConnectionState() == ConnectionState::kUnconnected) {
    return;
  }

  SetConnectionState(ConnectionState::kUnconnected);
  SetDoConnectTimestamp(0);
  SetEstablishTimestamp(0);

  read_buffer_.buffer.Clear();
  read_buffer_.builder.Clear();

  for (auto& msg : io_msgs_) {
    GetConnectionHandler()->NotifyMessageFailed(msg, ConnectionErrorCode::kNetworkException);
  }
  io_msgs_.clear();

  // Requirements: destroy IO-handler before close socket.
  GetIoHandler()->Destroy();

  DisableReadWrite();

  handshake_status_ = IoHandler::HandshakeStatus::kFailed;
  send_data_size_ = 0;
  need_direct_write_ = false;

  GetConnectionHandler()->ConnectionClosed();

  if (destroy) {
    GetConnectionHandler()->CleanResource();
  }
}

void TcpConnection::MessageWriteDone(IoMessage& msg) {
  GetConnectionHandler()->MessageWriteDone(msg);
  GetConnectionHandler()->SetCurrentContextExt(msg.context_ext);
}

int TcpConnection::Send(IoMessage&& msg) {
  if (TRPC_UNLIKELY(GetConnectionState() == ConnectionState::kConnecting)) {
    // protect the size of the io msg queue to prevent the queue too long
    if (io_msgs_.size() >= kMaxIoMsgNum) {
      TRPC_FMT_ERROR_EVERY_SECOND("Io message size exceeds the limit when connecting");
      return -1;
    }
  }

  send_data_size_ += msg.buffer.ByteSize();
  io_msgs_.emplace_back(std::move(msg));

  if (GetConnectionState() == ConnectionState::kConnecting) {
    return HandleWriteEvent();
  }

  if (notify_cache_msg_in_queue_) {
    return 0;
  }

  if (send_data_size_ >= kMergeSendDataSize || io_msgs_.size() >= kSendDataMergeNum) {
    return HandleWriteEvent();
  }

  if (need_direct_write_) {
    UpdateWriteEvent();
    need_direct_write_ = false;
  }

  return 0;
}

int TcpConnection::JudgeConnected() {
  int val = 0;
  auto len = static_cast<socklen_t>(sizeof(int));
  if (::getsockopt(socket_.GetFd(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&val), &len) == -1 || val) {
    return -1;
  }

  return 0;
}

bool TcpConnection::PreCheckOnWrite() {
  if (GetConnectionState() == ConnectionState::kConnecting) {
    if (JudgeConnected() != 0) {
      TRPC_LOG_ERROR("TcpConnection::HandleWriteEvent fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                           << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                           << ", close.");
      HandleClose(true);
      return false;
    }

    struct tcp_info info;
    int len = sizeof(info);
    if (getsockopt(socket_.GetFd(), IPPROTO_TCP, TCP_INFO, &info, reinterpret_cast<socklen_t*>(&len)) == -1) {
      TRPC_FMT_ERROR("Getsockopt of tcp_info failed, ip: {}, port: {}, is_client: {}", GetPeerIp(), GetPeerPort(),
                     IsClient());
      HandleClose(true);
      return false;
    }

    if (info.tcpi_state == TCP_ESTABLISHED) {
      TRPC_LOG_TRACE("Fd:" << socket_.GetFd() << ", ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                           << ", is_client:" << IsClient() << ", connect ok.");
      SetEstablishTimestamp(trpc::time::GetMilliSeconds());
      SetConnectionState(ConnectionState::kConnected);

      GetConnectionHandler()->ConnectionEstablished();
    }

    Handshaking(false);
    if (handshake_status_ != IoHandler::HandshakeStatus::kSucc) {
      return false;
    }

    if (info.tcpi_state == TCP_ESTABLISHED) {
      notify_cache_msg_in_queue_ = true;

      GetConnectionHandler()->NotifySendMessage();

      notify_cache_msg_in_queue_ = false;
    }
  } else {
    Handshaking(false);
    if (handshake_status_ != IoHandler::HandshakeStatus::kSucc) {
      return false;
    }
  }

  need_direct_write_ = true;

  return true;
}

// return: -1, close connection
int TcpConnection::ReadIoData(NoncontiguousBuffer& buff) {
  int ret = 0;
  while (true) {
    size_t writable_size = read_buffer_.builder.SizeAvailable();
    int n = GetIoHandler()->Read(read_buffer_.builder.data(), writable_size);
    if (n > 0) {
      ret += n;
      buff.Append(read_buffer_.builder.Seal(n));

      if ((size_t)n < writable_size) {
        break;
      }
    } else if (n == 0) {
      ret = -1;
      TRPC_LOG_DEBUG("TcpConnection::HandleReadEvent fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                          << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                          << ", recv empty packet and connection close.");
      HandleClose(true);
      break;
    } else {
      if (errno != EAGAIN) {
        if (errno == EINTR) {
          continue;
        }
        ret = n;
        TRPC_LOG_WARN("TcpConnection::ReadIoData fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                      << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                      << ", errno:" << errno << ", read failed and connection close.");
        HandleClose(true);
      }
      break;
    }
  }

  return ret;
}

void TcpConnection::UpdateWriteEvent() { reactor_->Update(this); }

}  // namespace trpc
