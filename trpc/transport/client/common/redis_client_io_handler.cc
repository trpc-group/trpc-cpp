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

#include "trpc/transport/client/common/redis_client_io_handler.h"

#include <iostream>
#include <sstream>
#include <utility>

#include "trpc/client/redis/cmdgen.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc {

const char kRedisSuccessReply[] = "+OK\r\n";

const char kRedisNoneedAuthReply[] =
    "-ERR AUTH <password> called without any password configured for the default user. Are you "
    "sure your configuration is correct?\r\n";

constexpr int kRedisSuccessReplyLen = 5;

RedisClientIoHandler::RedisClientIoHandler(Connection* conn, TransInfo* trans_info) : conn_(conn) {
  TRPC_ASSERT(conn_ != nullptr && "conn_ cannot be nullptr");
  TRPC_ASSERT(trans_info != nullptr && "trans_info cannot be nullptr");

  redis_conf_ = std::any_cast<RedisClientConf>(trans_info->user_data);

  // if not enable means do not exist RedisClientConf in *.yaml file,
  // we consider this as normal client-io-handler(just Read()、Write(), and Handshake() always return true)
  if (!redis_conf_.enable) {
    init_stage_ = RedisClientStage::kInit;
    return;
  }

  TRPC_ASSERT(!(redis_conf_.password.empty() && redis_conf_.db == 0) &&
              "forbid redis password is empty and db index is 0 at the same time");
  init_stage_ = RedisClientStage::kInit;

  if (!redis_conf_.password.empty()) {
    auth_cmd_ = trpc::redis::cmdgen{}.auth(redis_conf_.user_name, redis_conf_.password);
  } else {
    init_stage_ = RedisClientStage::kRecvAuthSucc;
  }

  if (redis_conf_.db != 0) {
    db_index_ = redis_conf_.db;
    select_cmd_ = trpc::redis::cmdgen{}.select(redis_conf_.db);
  }

  current_stage_ = init_stage_;
}

RedisClientIoHandler::~RedisClientIoHandler() { pos_ = 0; }

IoHandler::HandshakeStatus RedisClientIoHandler::Handshake(bool is_read_event) {
  if (!redis_conf_.enable) {
    // No RedisClientConf, we consider Handshake success
    return IoHandler::HandshakeStatus::kSucc;
  }

  IoHandler::HandshakeStatus ret = IoHandler::HandshakeStatus::kFailed;
  if (is_read_event) {
    do {
      ret = HandleRedisHandshake(is_read_event);
    } while (ret != IoHandler::HandshakeStatus::kSucc && ret != IoHandler::HandshakeStatus::kFailed &&
             ret != IoHandler::HandshakeStatus::kNeedRead);

  } else {
    ret = HandleRedisHandshake(is_read_event);
  }
  return ret;
}

IoHandler::HandshakeStatus RedisClientIoHandler::HandleRedisHandshake(bool is_read_event) {
  IoHandler::HandshakeStatus result = IoHandler::HandshakeStatus::kFailed;

  if (TRPC_UNLIKELY(conn_->GetFd() < 0)) {
    TRPC_LOG_DEBUG("Redis handshake fail. Stage is : " << static_cast<int>(this->current_stage_) << " , fd : "
                                                       << conn_->GetFd() << ", is_read_event:" << is_read_event);
    return result;
  }
  switch (this->current_stage_) {
    case RedisClientStage::kInit:
    case RedisClientStage::kSendAuthPart:
      result = SendAuthRequest();
      break;
    case RedisClientStage::kSendAuthSucc:
    case RedisClientStage::kRecvAuthPart:
      result = HandleAuthResponse(is_read_event);
      break;
    case RedisClientStage::kRecvAuthSucc:
    case RedisClientStage::kSendSelectPart:
      result = SendSelectRequest();
      break;
    case RedisClientStage::kSendSelectSucc:
    case RedisClientStage::kRecvSelectPart:
      result = HandleSelectResponse(is_read_event);
      break;
    case RedisClientStage::kRecvSelectSucc:
      result = IoHandler::HandshakeStatus::kSucc;
      break;
    default:
      pos_ = 0;
      break;
  }
  return result;
}

IoHandler::HandshakeStatus RedisClientIoHandler::SendAuthRequest() {
  IoHandler::HandshakeStatus result = IoHandler::HandshakeStatus::kFailed;
  if (this->current_stage_ == RedisClientStage::kInit) {
    pos_ = 0;
  }

  result = SendRequest(auth_cmd_);
  if (result == IoHandler::HandshakeStatus::kFailed) {
    TRPC_LOG_ERROR("SendAuthRequest fail. cmd :" << auth_cmd_
                                                 << ", Stage is : " << static_cast<int>(this->current_stage_)
                                                 << " , fd : " << conn_->GetFd());
    return result;
  }
  if (result == IoHandler::HandshakeStatus::kNeedWrite) {
    this->current_stage_ = RedisClientStage::kSendAuthPart;
  } else {
    this->current_stage_ = RedisClientStage::kSendAuthSucc;
  }
  return result;
}

IoHandler::HandshakeStatus RedisClientIoHandler::SendRequest(const std::string& cmd) {
  IoHandler::HandshakeStatus result = IoHandler::HandshakeStatus::kFailed;
  // Package request and send
  int auth_req_len = cmd.size() - pos_;
  struct iovec iovec_arr[1];
  iovec_arr[0].iov_base = reinterpret_cast<void*>(const_cast<char*>(cmd.c_str() + pos_));
  iovec_arr[0].iov_len = auth_req_len;
  int ret = Writev(iovec_arr, 1);
  if (TRPC_UNLIKELY(ret < 0)) {
    if (TRPC_UNLIKELY(errno == EAGAIN)) {
      result = IoHandler::HandshakeStatus::kNeedWrite;
      return result;
    } else {
      Reset();
      return result;
    }
  }
  if (TRPC_UNLIKELY(ret < auth_req_len)) {
    pos_ += ret;
    result = IoHandler::HandshakeStatus::kNeedWrite;
  } else {
    result = IoHandler::HandshakeStatus::kNeedRead;
  }
  return result;
}

IoHandler::HandshakeStatus RedisClientIoHandler::SendSelectRequest() {
  if (this->current_stage_ == RedisClientStage::kRecvAuthSucc && db_index_ == 0) {
    return IoHandler::HandshakeStatus::kSucc;
  }
  IoHandler::HandshakeStatus result = IoHandler::HandshakeStatus::kFailed;

  if (this->current_stage_ == RedisClientStage::kRecvAuthSucc) {
    pos_ = 0;
  }
  result = SendRequest(select_cmd_);
  if (result == IoHandler::HandshakeStatus::kFailed) {
    TRPC_LOG_ERROR("SendSelectRequest fail. select_cmd_ :" << select_cmd_
                                                           << ", Stage is : " << static_cast<int>(this->current_stage_)
                                                           << " , fd : " << conn_->GetFd());
    return result;
  }

  if (result == IoHandler::HandshakeStatus::kNeedWrite) {
    this->current_stage_ = RedisClientStage::kSendSelectPart;
  } else {
    this->current_stage_ = RedisClientStage::kSendSelectSucc;
  }
  return result;
}

IoHandler::HandshakeStatus RedisClientIoHandler::HandleSelectResponse(bool is_read_event) {
  if (!is_read_event) {
    return IoHandler::HandshakeStatus::kNeedRead;
  }
  IoHandler::HandshakeStatus result = IoHandler::HandshakeStatus::kFailed;
  if (this->current_stage_ == RedisClientStage::kSendSelectSucc) {
    pos_ = 0;
  }
  char recv_buff[128] = {0};
  int recv_len = RecvResponse(recv_buff);
  if (recv_len < 0) {
    TRPC_LOG_ERROR("HandleSelectResponse fail. Stage is : " << static_cast<int>(this->current_stage_)
                                                            << " , fd : " << conn_->GetFd());
    return result;
  }
  if (recv_len == 0) {
    this->current_stage_ = RedisClientStage::kRecvSelectPart;
    return IoHandler::HandshakeStatus::kNeedRead;
  }
  if (UnPackageSelectResponse(recv_buff, recv_len) != 0) {
    Reset();
    return result;
  }

  if (this->current_stage_ == RedisClientStage::kRecvSelectSucc) {
    result = IoHandler::HandshakeStatus::kSucc;
  } else {
    result = IoHandler::HandshakeStatus::kNeedRead;
  }
  return result;
}

int RedisClientIoHandler::RecvResponse(char* recv_buff) {
  const int read_len = 128;
  int ret = Read(recv_buff, read_len);
  if (TRPC_UNLIKELY(ret <= 0)) {
    if (TRPC_UNLIKELY(errno == EAGAIN)) {
      return 0;
    } else {
      Reset();
      return -1;
    }
  }
  return ret;
}

IoHandler::HandshakeStatus RedisClientIoHandler::HandleAuthResponse(bool is_read_event) {
  if (!is_read_event) {
    return IoHandler::HandshakeStatus::kNeedRead;
  }
  IoHandler::HandshakeStatus result = IoHandler::HandshakeStatus::kFailed;
  if (this->current_stage_ == RedisClientStage::kSendAuthSucc) {
    pos_ = 0;
  }

  char recv_buff[128] = {0};

  int recv_len = RecvResponse(recv_buff);
  if (recv_len < 0) {
    TRPC_LOG_ERROR("HandleAuthResponse fail. Stage is : " << static_cast<int>(this->current_stage_)
                                                          << " , fd : " << conn_->GetFd());
    return result;
  }
  if (recv_len == 0) {
    this->current_stage_ = RedisClientStage::kRecvAuthPart;
    return IoHandler::HandshakeStatus::kNeedRead;
  }

  if (UnPackageAuthResponse(recv_buff, recv_len) != 0) {
    Reset();
    return result;
  }

  if (this->current_stage_ == RedisClientStage::kRecvAuthSucc && this->db_index_ == 0) {
    result = IoHandler::HandshakeStatus::kSucc;
  } else if (this->current_stage_ == RedisClientStage::kRecvAuthSucc) {
    result = IoHandler::HandshakeStatus::kNeedWrite;
  } else {
    result = IoHandler::HandshakeStatus::kNeedRead;
  }
  return result;
}

int RedisClientIoHandler::UnPackageSelectResponse(char* recv_buff, const int recv_len) {
  if (TRPC_UNLIKELY(recv_len + pos_ > kRedisSuccessReplyLen)) {
    return -1;
  }
  if (TRPC_LIKELY(strncmp(recv_buff, kRedisSuccessReply + pos_, recv_len) == 0)) {
    if (recv_len + pos_ == kRedisSuccessReplyLen) {
      this->current_stage_ = RedisClientStage::kRecvSelectSucc;
    } else {
      this->current_stage_ = RedisClientStage::kRecvSelectPart;
    }
    pos_ += recv_len;
    return 0;
  }
  std::string error_msg(recv_buff, recv_len);
  TRPC_LOG_ERROR("UnPackageSelectResponse fail. Stage is : " << static_cast<int>(this->current_stage_) << " , fd : "
                                                             << conn_->GetFd() << ",error_msg:" << error_msg);
  return -1;
}

int RedisClientIoHandler::UnPackageAuthResponse(char* recv_buff, const int recv_len) {
  const int noneed_auth_reply_len = 127;
  if (TRPC_UNLIKELY(recv_len + pos_ > noneed_auth_reply_len)) {
    TRPC_LOG_ERROR("UnPackageAuthResponse fail. Stage is : " << static_cast<int>(this->current_stage_)
                                                             << " , fd : " << conn_->GetFd());
    return -1;
  }

  // if reply length > auth_success_reply_len_,it only can be noneed_auth_reply_
  // else can be auth_success_reply_ noneed_auth_reply_
  if (TRPC_UNLIKELY(recv_len + pos_ > kRedisSuccessReplyLen)) {
    if (strncmp(recv_buff, kRedisNoneedAuthReply + pos_, recv_len) == 0) {
      if (recv_len + pos_ == noneed_auth_reply_len) {
        this->current_stage_ = RedisClientStage::kRecvAuthSucc;
      } else {
        this->current_stage_ = RedisClientStage::kRecvAuthPart;
      }
      pos_ += recv_len;
      return 0;
    } else {
      std::string error_msg(recv_buff, recv_len);
      TRPC_LOG_ERROR("UnPackageAuthResponse fail. Stage is : " << static_cast<int>(this->current_stage_) << " , fd : "
                                                               << conn_->GetFd() << ",error_msg:" << error_msg);
      return -1;
    }
  }

  if (TRPC_LIKELY(strncmp(recv_buff, kRedisSuccessReply + pos_, recv_len) == 0)) {
    if (recv_len + pos_ == kRedisSuccessReplyLen) {
      this->current_stage_ = RedisClientStage::kRecvAuthSucc;
    } else {
      this->current_stage_ = RedisClientStage::kRecvAuthPart;
    }
    pos_ += recv_len;
    return 0;
  }
  if (strncmp(recv_buff, kRedisNoneedAuthReply + pos_, recv_len) == 0) {
    this->current_stage_ = RedisClientStage::kRecvAuthPart;
    pos_ += recv_len;
    return 0;
  }
  TRPC_LOG_ERROR("UnPackageAuthResponse fail. Stage is : " << static_cast<int>(this->current_stage_)
                                                           << " , fd : " << conn_->GetFd());
  return -1;
}

int RedisClientIoHandler::Reset() {
  pos_ = 0;
  this->current_stage_ = init_stage_;
  return 0;
}

}  // namespace trpc
