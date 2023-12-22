//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/native/stream_connection.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"

#include <deque>
#include <limits>
#include <utility>
#include <vector>

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

using namespace std::literals;

namespace trpc {

FiberTcpConnection::FiberTcpConnection(Reactor* reactor, const Socket& socket)
    : FiberConnection(reactor), socket_(socket) {
  TRPC_ASSERT(socket_.IsValid());

  SetFd(socket_.GetFd());
}

FiberTcpConnection::~FiberTcpConnection() {
  // Requirements: destroy IO-handler before close socket.
  GetIoHandler()->Destroy();
  socket_.Close();

  TRPC_LOG_DEBUG("~FiberTcpConnection fd:" << socket_.GetFd() << ", conn_id:" << this->GetConnId());
  TRPC_ASSERT(!socket_.IsValid());
}

void FiberTcpConnection::Established() {
  SetEstablishTimestamp(trpc::time::GetMilliSeconds());
  SetConnectionState(ConnectionState::kConnected);
  SetConnActiveTime(trpc::time::GetMilliSeconds());

  TRPC_ASSERT(GetIoHandler());
  TRPC_ASSERT(GetConnectionHandler());

  GetConnectionHandler()->ConnectionEstablished();

  AttachReactor();

  TRPC_LOG_TRACE("FiberTcpConnection::Established fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                       << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                       << ", conn_id: " << this->GetConnId() << ", Connect ok.");
}

bool FiberTcpConnection::DoConnect() {
  TRPC_ASSERT(GetConnectionState() == ConnectionState::kUnconnected);
  TRPC_ASSERT(socket_.IsValid());

  SetDoConnectTimestamp(trpc::time::GetMilliSeconds());

  NetworkAddress remote_addr(GetPeerIp(), GetPeerPort(), GetPeerIpType());

  int ret = socket_.Connect(remote_addr);
  if (ret == 0) {
    TRPC_LOG_DEBUG("FiberTcpConnection::DoConnect target: " << remote_addr.ToString() << ", fd: " << socket_.GetFd()
                                                            << ", is_client:" << IsClient()
                                                            << ", conn_id: " << this->GetConnId() << ", progress.");

    handshaking_state_.done.store(false, std::memory_order_relaxed);

    Established();

    return true;
  }

  TRPC_LOG_ERROR("FiberTcpConnection::DoConnect target: " << remote_addr.ToString() << ", fd: " << socket_.GetFd()
                                                          << ", is_client:" << IsClient()
                                                          << ", conn_id: " << this->GetConnId() << ", failed.");

  GetIoHandler()->Destroy();
  socket_.Close();

  return false;
}

void FiberTcpConnection::StartHandshaking() {
  auto status = DoHandshake(false);
  if (status == IoHandler::HandshakeStatus::kFailed) {
    Kill(CleanupReason::kHandshakeFailed);
  } else if (status == IoHandler::HandshakeStatus::kNeedRead) {
    RestartReadIn(0ns);
  } else if (status == IoHandler::HandshakeStatus::kNeedWrite) {
    RestartWriteIn(0ns);
  }
}

int FiberTcpConnection::Send(IoMessage&& msg) {
  if (!Enabled()) {
    return -1;
  }

  auto append_status =
      writing_buffers_.Append(std::move(msg.buffer), std::move(msg), GetSendQueueCapacity(), GetSendQueueTimeout());
  if (TRPC_LIKELY(append_status == WritingBufferList::kAppendHead)) {
    if (TRPC_UNLIKELY(!handshaking_state_.done.load(std::memory_order_relaxed))) {
      std::scoped_lock _(handshaking_state_.lock);
      if (!handshaking_state_.done.load(std::memory_order_relaxed)) {
        TRPC_ASSERT(!handshaking_state_.pending_restart_writes);
        handshaking_state_.pending_restart_writes = true;
        return 0;
      }
    }

    constexpr auto kMaximumBytesPerCall = 1048576;
    auto flush_status = FlushWritingBuffer(kMaximumBytesPerCall);
    if (TRPC_LIKELY(flush_status == FlushStatus::kFlushed)) {
      return 0;
    } else if (flush_status == FlushStatus::kSystemBufferSaturated || flush_status == FlushStatus::kQuotaExceeded) {
      RestartWriteIn(0ms);
      return 0;
    } else if (flush_status == FlushStatus::kPartialWrite || flush_status == FlushStatus::kError) {
      TRPC_LOG_WARN("FiberTcpConnection::Send failed to write ip:"
                     << GetPeerIp() << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                     << ", conn_id: " << this->GetConnId() << ", flush_status:" << static_cast<int>(flush_status));
      Kill(CleanupReason::kError);
      return -1;
    } else if (flush_status == FlushStatus::kNothingWritten) {
      TRPC_LOG_WARN("FiberTcpConnection::Send failed to write ip:"
                     << GetPeerIp() << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                     << ", conn_id: " << this->GetConnId() << ", flush_status:" << static_cast<int>(flush_status));
      Kill(CleanupReason::kDisconnect);
      return -1;
    } else {
      TRPC_LOG_ERROR("FiberTcpConnection::Send exception to write ip:"
                     << GetPeerIp() << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                     << ", conn_id: " << this->GetConnId() << ", flush_status:" << static_cast<int>(flush_status));
      TRPC_ASSERT(false);
    }
  } else if (append_status == WritingBufferList::kTimeout) {
    TRPC_LOG_ERROR("FiberTcpConnection::Send timeout to write ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                                                                   << ", is_client:" << IsClient()
                                                                   << ", conn_id: " << this->GetConnId());
    return -2;
  }

  return 0;
}

void FiberTcpConnection::DoClose(bool destroy) { Stop(); }

void FiberTcpConnection::Stop() {
  TRPC_LOG_DEBUG("FiberTcpConnection::Stop fd:" << socket_.GetFd() << ", ip:" << GetPeerIp()
                                                << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                                                << ", conn_id:" << this->GetConnId());
  Kill(CleanupReason::UserInitiated);
}

void FiberTcpConnection::Join() { WaitForCleanup(); }

FiberConnection::EventAction FiberTcpConnection::OnReadable() {
  if (!Enabled()) {
    return EventAction::kLeaving;
  }

  if (TRPC_UNLIKELY(!handshaking_state_.done.load(std::memory_order_relaxed))) {
    auto action = DoHandshake(true);
    if (action == IoHandler::HandshakeStatus::kFailed) {
      Kill(CleanupReason::kError);
      return EventAction::kLeaving;
    } else if (action == IoHandler::HandshakeStatus::kNeedRead) {
      return EventAction::kReady;
    } else if (action == IoHandler::HandshakeStatus::kNeedWrite) {
      RestartWriteIn(0ns);
      return EventAction::kSuppress;
    } else {
      TRPC_ASSERT(action == IoHandler::HandshakeStatus::kSucc);
    }
  }
  TRPC_ASSERT(handshaking_state_.done.load(std::memory_order_relaxed));

  ReadStatus status;
  do {
    status = ReadData();
    if (TRPC_LIKELY(status != ReadStatus::kError)) {
      auto rc = ConsumeReadData();
      if (TRPC_UNLIKELY(rc != EventAction::kReady)) {
        TRPC_LOG_WARN("FiberTcpConnection::OnReadable ConsumeReadData failed, ip:"
                       << GetPeerIp() << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                       << ", conn_id:" << this->GetConnId());
        Kill(CleanupReason::kError);
        return rc;
      }
    }
  } while (status == ReadStatus::kPartialRead);

  if (TRPC_LIKELY(status == ReadStatus::kDrained)) {
    return EventAction::kReady;
  } else if (status == ReadStatus::kRemoteClose) {
    TRPC_LOG_DEBUG("FiberTcpConnection::OnReadable remote close, ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                                                                      << ", is_client:" << IsClient()
                                                                      << ", conn_id:" << this->GetConnId());
    Kill(CleanupReason::kDisconnect);
    return EventAction::kLeaving;
  } else {
    TRPC_LOG_WARN("FiberTcpConnection::OnReadable ReadData failed, ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                                                                         << ", is_client:" << IsClient()
                                                                         << ", conn_id:" << this->GetConnId());
    Kill(CleanupReason::kError);
    return EventAction::kLeaving;
  }
}

FiberTcpConnection::ReadStatus FiberTcpConnection::ReadData() {
  size_t recv_buffer_size = GetRecvBufferSize();
  size_t total_read = 0;
  while (true) {
    size_t writable_size = read_buffer_.builder.SizeAvailable();
    if (int n = GetIoHandler()->Read(read_buffer_.builder.data(), writable_size); n > 0) {
      read_buffer_.buffer.Append(read_buffer_.builder.Seal(n));

      if (size_t read = n; read < writable_size) {
        return ReadStatus::kDrained;
      } else if (recv_buffer_size != 0 && (total_read += read) >= recv_buffer_size) {
        return ReadStatus::kPartialRead;
      }
    } else if (n == 0) {
      return ReadStatus::kRemoteClose;
    } else {
      return errno != EAGAIN ? ReadStatus::kError : ReadStatus::kDrained;
    }
  }
}

FiberConnection::EventAction FiberTcpConnection::ConsumeReadData() {
  if (read_buffer_.buffer.ByteSize() <= 0) {
    return EventAction::kReady;
  }

  RefPtr ref(ref_ptr, this);
  std::deque<std::any> data;
  int checker_ret = GetConnectionHandler()->CheckMessage(ref, read_buffer_.buffer, data);
  if (checker_ret == kPacketFull) {
    if (!GetConnectionHandler()->HandleMessage(ref, data)) {
      return EventAction::kLeaving;
    }

    SetConnActiveTime(trpc::time::GetMilliSeconds());
    GetConnectionHandler()->UpdateConnection();

    return EventAction::kReady;
  } else if (checker_ret == kPacketError) {
    return EventAction::kLeaving;
  }

  return EventAction::kReady;
}

FiberConnection::EventAction FiberTcpConnection::OnWritable() {
  if (!Enabled()) {
    return EventAction::kLeaving;
  }

  if (TRPC_UNLIKELY(!handshaking_state_.done.load(std::memory_order_relaxed))) {
    auto action = DoHandshake(false);
    if (action == IoHandler::HandshakeStatus::kFailed) {
      Kill(CleanupReason::kError);
      return EventAction::kLeaving;
    } else if (action == IoHandler::HandshakeStatus::kNeedWrite) {
      return EventAction::kReady;
    } else if (action == IoHandler::HandshakeStatus::kNeedRead) {
      RestartReadIn(0ns);
      return EventAction::kSuppress;
    } else {
      TRPC_ASSERT(action == IoHandler::HandshakeStatus::kSucc);
    }
  }
  TRPC_ASSERT(handshaking_state_.done.load(std::memory_order_relaxed));

  auto status = FlushWritingBuffer(std::numeric_limits<std::size_t>::max());
  if (status == FlushStatus::kSystemBufferSaturated) {
    return EventAction::kReady;
  } else if (status == FlushStatus::kFlushed) {
    return EventAction::kSuppress;
  } else if (status == FlushStatus::kPartialWrite || status == FlushStatus::kNothingWritten) {
    Kill(CleanupReason::kDisconnect);
    return EventAction::kLeaving;
  } else if (status == FlushStatus::kError) {
    Kill(CleanupReason::kError);
    return EventAction::kLeaving;
  } else {
    TRPC_ASSERT(false);
  }
}

FiberTcpConnection::FlushStatus FiberTcpConnection::FlushWritingBuffer(std::size_t max_bytes) {
  auto bytes_quota = max_bytes;
  bool ever_succeeded = false;

  while (bytes_quota) {
    bool emptied = false;
    bool short_write = false;
    auto written = writing_buffers_.FlushTo(GetIoHandler(), GetConnectionHandler(), bytes_quota,
                                            GetSendQueueCapacity(), SupportPipeline(),
                                            &emptied, &short_write);
    if (TRPC_UNLIKELY(written == 0 && !emptied)) {
      TRPC_LOG_WARN("FiberTcpConnection::FlushWritingBuffer write ip:"
                     << GetPeerIp() << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                     << ", conn_id: " << this->GetConnId() << ", remote peer has closed the connection.");
      return ever_succeeded ? FlushStatus::kPartialWrite : FlushStatus::kNothingWritten;
    }
    if (TRPC_UNLIKELY(written < 0)) {
      if (errno == EAGAIN) {
        return FlushStatus::kSystemBufferSaturated;
      } else {
        TRPC_LOG_WARN("FiberTcpConnection::FlushWritingBuffer write ip:"
                       << GetPeerIp() << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                       << ", conn_id: " << this->GetConnId() << ", failed because socket close.");
        return ever_succeeded ? FlushStatus::kError : FlushStatus::kNothingWritten;
      }
    }
    TRPC_ASSERT(static_cast<std::size_t>(written) <= bytes_quota);

    ever_succeeded = true;
    bytes_quota -= written;

    // Update the active time of the connection when there is a write operation on the file descriptor (fd)
    SetConnActiveTime(trpc::time::GetMilliSeconds());
    GetConnectionHandler()->UpdateConnection();

    TRPC_ASSERT(!(short_write && emptied));
    if (emptied) {
      return FlushStatus::kFlushed;
    }
    if (short_write) {
      return FlushStatus::kSystemBufferSaturated;
    }
  }

  return FlushStatus::kQuotaExceeded;
}

void FiberTcpConnection::OnError(int err) {
  TRPC_LOG_DEBUG("FiberTcpConnection::OnError ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                                                   << ", fd: " << socket_.GetFd() << ", is_client:" << IsClient()
                                                   << ", conn_id:" << this->GetConnId() << ", err:" << err);
  Kill(CleanupReason::kError);
}

void FiberTcpConnection::OnCleanup(CleanupReason reason) {
  TRPC_ASSERT(reason != CleanupReason::kNone);

  TRPC_LOG_DEBUG("FiberTcpConnection::OnCleanup ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                                                     << ", is_client:" << IsClient() << ", fd: " << socket_.GetFd()
                                                     << ", conn_id:" << this->GetConnId()
                                                     << ", reason:" << static_cast<int>(reason));

  // When the stream is closed, it is necessary to properly close the stream according to the reason for the closure.
  // If the connection is closed normally (the local end actively closes the connection), it is necessary to wait for
  // the current stream to be processed before handling it.
  // If the connection is closed abnormally (the remote end directly closes the connection), the remote end connection
  // is no longer writable, so there is no need to process the current stream.
  cleanup_reason_ = reason;

  GetConnectionHandler()->ConnectionClosed();
  GetConnectionHandler()->CleanResource();

  writing_buffers_.Stop();

  // For multi-threads-safety, move "socket_.Close()" to ~FiberTcpConnection();
}

IoHandler::HandshakeStatus FiberTcpConnection::DoHandshake(bool from_on_readable) {
  std::scoped_lock _(handshaking_state_.lock);
  if (handshaking_state_.done.load(std::memory_order_relaxed)) {
    return IoHandler::HandshakeStatus::kSucc;
  }

  auto status = GetIoHandler()->Handshake(from_on_readable);
  if (status == IoHandler::HandshakeStatus::kFailed) {
    TRPC_LOG_ERROR("FiberTcpConnection::DoHandshake failed, ip:"
                   << GetPeerIp() << ", port:" << GetPeerPort() << ", is_client:" << IsClient()
                   << ", fd: " << socket_.GetFd() << ", conn_id:" << this->GetConnId());
    return status;
  } else if (status == IoHandler::HandshakeStatus::kSucc) {
    if (handshaking_state_.need_restart_read) {
      // The last non-terminal status returned by us was `WannaWrite`, in this
      // case the caller should have suppressed read event. Reenable it then.
      TRPC_ASSERT(!from_on_readable);
      RestartReadIn(0ns);
    }
    if (handshaking_state_.pending_restart_writes) {
      // Someone tried to write but suspended as handshake hadn't done at that
      // time, restart that operation then.
      if (from_on_readable) {  // If we're acting on an write event, simply
                               // asking the caller to fallthrough is enough.
        RestartWriteIn(0ns);
      }
    }
    // If `OnReadable` / `OnWritable` (enabled above) comes before we've finally
    // updated `handshaking_state_.done`, they will call us first. In this case,
    // the test at the beginning of this method will see the update (after we've
    // released the lock here) and return `kSucc` correctly.
    handshaking_state_.done.store(true, std::memory_order_relaxed);  // release?
    return status;
  } else if (status == IoHandler::HandshakeStatus::kNeedWrite) {
    // Returning `kNeedWrite` would make the caller to suppress `Event::Read`.
    // However, read event is always interested once handshake is done. So we
    // leave a mark here, and re-enable read event once handshake is done.
    handshaking_state_.need_restart_read = true;
    return status;
  } else {
    TRPC_ASSERT(status == IoHandler::HandshakeStatus::kNeedRead);
    handshaking_state_.need_restart_read = false;
    return status;
  }
}

}  // namespace trpc
