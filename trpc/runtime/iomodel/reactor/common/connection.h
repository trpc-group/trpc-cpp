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

#include <any>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/event_handler.h"

namespace trpc {

/// @brief Base class for tcp connection and udp transceiver
class alignas(8) Connection : public EventHandler {
 public:
  /// @brief Init
  virtual bool Initialize() { return false; }

  /// @brief Hand-shake
  virtual void StartHandshaking() {}

  /// @brief Connection established successfully
  virtual void Established() {}

  /// @brief Client begin to establish connection
  virtual bool DoConnect() { return false; }

  /// @brief Send message
  virtual int Send(IoMessage&& msg) { return -1; }

  /// @brief Close the connection
  /// @param true: this connection object is deleted by ConnectionCleanFunction
  ///        false: this connection object needs to be actively deleted by the upper layer
  virtual void DoClose(bool destroy) {}

  /// @brief Get/Set iohandler
  IoHandler* GetIoHandler() const { return io_handler_.get(); }
  void SetIoHandler(std::unique_ptr<IoHandler> io_handler) { io_handler_ = std::move(io_handler); }

  /// @brief Get/Set connection handler
  ConnectionHandler* GetConnectionHandler() const { return conn_handler_.get(); }
  void SetConnectionHandler(std::unique_ptr<ConnectionHandler> conn_handler) {
    conn_handler_ = std::move(conn_handler);
  }

  /// @brief Get/Set connection type
  ConnectionType GetConnType() const { return conn_type_; }
  void SetConnType(ConnectionType type) { conn_type_ = type; }

  /// @brief Get/Set connection state
  ConnectionState GetConnectionState() const { return state_; }
  void SetConnectionState(ConnectionState state) { state_ = state; }

  /// @brief Get/Set the connection that belongs to the client or the server
  bool IsClient() const { return is_client_; }
  void SetClient() { is_client_ = true; }

  /// @brief Get/Set the timeout that check if the client connection has timed out
  uint32_t GetCheckConnectTimeout() const { return check_connect_timeout_; }
  void SetCheckConnectTimeout(uint32_t timeout) { check_connect_timeout_ = timeout; }

  /// @brief Get/Set connection id
  uint64_t GetConnId() const { return conn_id_; }
  void SetConnId(uint64_t conn_id) { conn_id_ = conn_id; }

  /// @brief Get/Set the context info in the connection active management list
  std::list<uint64_t>::iterator GetContext() const { return context_; }
  void SetContext(std::list<uint64_t>::iterator it) { context_ = it; }

  /// @brief Get/Set the time when the client begin to establish connection
  uint64_t GetDoConnectTimestamp() { return do_connect_timestamp_; }
  void SetDoConnectTimestamp(uint64_t timestamp) { do_connect_timestamp_ = timestamp; }

  /// @brief Get/Set the time when establish connection successfully
  uint64_t GetEstablishTimestamp() { return establish_timestamp_; }
  void SetEstablishTimestamp(uint64_t timestamp) { establish_timestamp_ = timestamp; }

  /// @brief Get/Set the connection active time
  void SetConnActiveTime(uint64_t time_ms) { active_time_ = time_ms; }
  uint64_t GetConnActiveTime() const { return active_time_; }

  /// @brief Get/Set peer ip
  std::string GetPeerIp() const { return peer_ip_; }
  void SetPeerIp(std::string&& peer_ip) { peer_ip_ = std::move(peer_ip); }

  /// @brief Get/Set peer port
  int GetPeerPort() const { return peer_port_; }
  void SetPeerPort(uint16_t peer_port) { peer_port_ = peer_port; }

  /// @brief Get/Set peer ip type
  NetworkAddress::IpType GetPeerIpType() const { return peer_ip_type_; }
  void SetPeerIpType(NetworkAddress::IpType type) { peer_ip_type_ = type; }

  /// @brief Get/Set local ip
  std::string GetLocalIp() const { return local_ip_; }
  void SetLocalIp(std::string&& local_ip) { local_ip_ = std::move(local_ip); }

  /// @brief Get/Set local port
  int GetLocalPort() const { return local_port_; }
  void SetLocalPort(uint16_t local_port) { local_port_ = local_port; }

  /// @brief Get/Set local ip type
  NetworkAddress::IpType GetLocalIpType() const { return local_ip_type_; }
  void SetLocalIpType(NetworkAddress::IpType type) { local_ip_type_ = type; }

  /// @brief Get/Set the size limit of a single request packet
  uint32_t GetMaxPacketSize() const { return max_packet_size_; }
  void SetMaxPacketSize(uint32_t max_packet_size) { max_packet_size_ = max_packet_size; }

  /// @brief Get/Set the buffer size limit for receiving network data each time
  uint32_t GetRecvBufferSize() const { return recv_buffer_size_; }
  void SetRecvBufferSize(uint32_t recv_buffer_size) { recv_buffer_size_ = recv_buffer_size; }

  /// @brief Get/Set the size limit of the send queue
  uint32_t GetSendQueueCapacity() const { return send_queue_capacity_; }
  void SetSendQueueCapacity(uint32_t send_queue_capacity) { send_queue_capacity_ = send_queue_capacity; }

  /// @brief Get/Set the wait timeout of the send queue when exceeded the queue limit
  uint32_t GetSendQueueTimeout() const { return send_queue_timeout_; }
  void SetSendQueueTimeout(uint32_t send_queue_timeout) { send_queue_timeout_ = send_queue_timeout; }

  /// @brief Get/Set self-define field
  std::any& GetUserAny() { return user_any_; }
  void SetUserAny(std::any&& user_data) { user_any_ = std::move(user_data); }

  /// @brief Get/Set whether support pipeline
  bool SupportPipeline() const { return support_pipeline_; }
  void SetSupportPipeline() { support_pipeline_ = true; }

  /// @brief Get the number of requests that merge together to send
  uint32_t GetMergeRequestCount() {
    if (conn_handler_) {
      return conn_handler_->GetMergeRequestCount();
    }
    return 1;
  }

 private:
  // Connection id
  uint64_t conn_id_;

  // The context info in the connection active management list
  // when the connection is not active(idle timeout), the connection will be closed
  std::list<uint64_t>::iterator context_;

  // Handle io operation on the connection
  std::unique_ptr<IoHandler> io_handler_{nullptr};

  // Manager connection data and state
  std::unique_ptr<ConnectionHandler> conn_handler_{nullptr};

  // The time when the client begin to establish connection
  uint64_t do_connect_timestamp_{0};

  // The time when establish connection successfully
  uint64_t establish_timestamp_{0};

  // Connection active time
  // this time will be updated when sending and receiving packets
  uint64_t active_time_{0};

  // The size limit of a single request packet
  uint32_t max_packet_size_{10000000};

  // The buffer size limit for receiving network data each time,
  // if exceeded, it will be directly submitted to the upper layer for processing
  // 0: not limited
  uint32_t recv_buffer_size_{1000000};

  // The size limit of the send queue(current fiber use)
  // when exceeded, the send operation will be blocked
  // 0: not limited
  uint32_t send_queue_capacity_{10000000};

  // The wait timeout of the send queue(current fiber use)
  // when send queue exceeded the limit
  uint32_t send_queue_timeout_{10000000};

  // The timeout that check if the client connection has timed out(ms)
  // default 0, not check
  uint32_t check_connect_timeout_{0};

  // Connecction state
  ConnectionState state_{ConnectionState::kUnconnected};

  // Connecction type
  ConnectionType conn_type_;

  // Connection whether belongs to the client or the server
  bool is_client_{false};

  // Whether support pipeline
  bool support_pipeline_{false};

  // Local port
  int local_port_{0};

  // Peer port
  int peer_port_{0};

  // Ip v4 or ip v6
  NetworkAddress::IpType local_ip_type_;

  // Ip v4 or ip v6
  NetworkAddress::IpType peer_ip_type_;

  // Local ip
  std::string local_ip_;

  // Peer ip
  std::string peer_ip_;

  // Self-define field
  std::any user_any_{nullptr};
};

}  // namespace trpc
