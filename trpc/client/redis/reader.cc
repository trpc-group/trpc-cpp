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

#include "trpc/client/redis/reader.h"

#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc {

namespace redis {

void Reader::Init() {
  ridx_ = -1;
  protocol_err_ = false;
  has_parse_seqno_ = false;

  for (int i = 0; i < 9; ++i) {
    rstack_[i].elements_ = 0;
    rstack_[i].idx_ = -1;
    rstack_[i].obj_ = nullptr;
    rstack_[i].parent_ = nullptr;
  }
}

bool Reader::TryReadable(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& pos) {
  if (in.Empty() || !(cur_itr != in.end())) {
    return false;
  }

  if (cur_itr->size() > pos) {
    return true;
  }

  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  ++tmp_itr;
  size_t tmp_pos = 0;
  if (TryReadable(in, tmp_itr, tmp_pos)) {
    cur_itr = tmp_itr;
    pos = tmp_pos;
    return true;
  }

  return false;
}

const char* Reader::SeekNewLine(const char* s, int len) {
  if (len < 2) {
    return nullptr;
  }
  char* p = const_cast<char*>(s);
  len--;
  char* ret = nullptr;
  while ((ret = static_cast<char*>(memchr(p, '\r', len))) != nullptr) {
    if (ret[1] == '\n') {
      break;
    }
    ret++;
    len -= ret - p;
    p = ret;
  }
  return ret;
}

int64_t Reader::ConvertToInteger(const char* s, size_t len) {
  int64_t v = 0;
  int dec, mult = 1;
  size_t count = len;
  char c;
  if (*s == '-') {
    mult = -1;
    count--;
    s++;
  } else if (*s == '+') {
    mult = 1;
    count--;
    s++;
  }
  while (count > 0) {
    c = *(s++);
    dec = c - '0';
    if (dec >= 0 && dec < 10) {
      v *= 10;
      v += dec;
    } else {
      // Should not happen...
      return -1;
    }
    count--;
  }
  return mult * v;
}

const char* Reader::ReadInteger(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& pos,
                                size_t& len) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  size_t tmp_len = 0;
  if (!TryReadVariableLengthString(in, tmp_itr, tmp_pos, tmp_len)) {
    return nullptr;
  }

  if (tmp_len >= sizeof(r_buf_)) {
    protocol_err_ = true;
    return nullptr;
  }

  const char* p = cur_itr->data() + pos;
  size_t real_read_size = cur_itr->size() - pos;
  if (real_read_size < tmp_len) {
    size_t wait_len = tmp_len;
    size_t buff_pos = 0;
    NoncontiguousBuffer::const_iterator next_itr = cur_itr;
    // It may span across multiple blocks
    while (wait_len) {
      if (wait_len <= real_read_size) {
        memcpy(r_buf_ + buff_pos, p, wait_len);
        p = r_buf_;
        break;
      }
      memcpy(r_buf_ + buff_pos, p, real_read_size);
      wait_len -= real_read_size;
      buff_pos += real_read_size;
      ++next_itr;
      p = next_itr->data();
      real_read_size = next_itr->size();
    }
  }
  cur_itr = tmp_itr;
  pos = tmp_pos;
  len = tmp_len;
  return p;
}

bool Reader::TryReadFixedLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                                      size_t& pos, const size_t fixed_len) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  // It contains two delimiters "\r\n"
  size_t left_len = fixed_len + 2;
  while (left_len > 0 && (tmp_itr != in.end())) {
    size_t real_read_size = tmp_itr->size() - tmp_pos;
    if (real_read_size > left_len) {
      cur_itr = tmp_itr;
      pos = tmp_pos + left_len;
      return true;
    }
    if (real_read_size == left_len) {
      ++tmp_itr;
      pos = 0;
      cur_itr = tmp_itr;
      return true;
    }

    // Keep looping
    left_len -= real_read_size;
    ++tmp_itr;
    tmp_pos = 0;
  }
  return false;
}

bool Reader::TryReadVariableLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                                         size_t& buf_pos, size_t& len) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = buf_pos;
  const char* p = tmp_itr->data() + tmp_pos;
  size_t real_read_size = tmp_itr->size() - tmp_pos;
  const char* s = Reader::SeekNewLine(p, real_read_size);
  bool is_end = false;
  if (s != nullptr) {
    len = s - p;
    tmp_pos += (len + 2);
    is_end = (tmp_pos >= tmp_itr->size());
    cur_itr = is_end ? ++tmp_itr : tmp_itr;
    buf_pos = is_end ? 0 : tmp_pos;
    return true;
  }

  // Data may span across multiple blocks
  NoncontiguousBuffer::const_iterator next_itr = tmp_itr;
  ++next_itr;
  size_t next_pos = 0;
  if (!TryReadable(in, next_itr, next_pos)) {
    return false;
  }

  // The block which next_itr MUST get data
  s = next_itr->data();
  if (p[real_read_size - 1] == '\r' && s[0] == '\n') {
    len = real_read_size - 1;
    is_end = (next_itr->size() == 1);
    buf_pos = is_end ? 0 : 1;
    cur_itr = is_end ? ++next_itr : next_itr;
    return true;
  }

  if (next_itr->size() >= 2 && s[0] == '\r' && s[1] == '\n') {
    is_end = (next_itr->size() == 2);
    len = real_read_size;
    buf_pos = is_end ? 0 : 2;
    cur_itr = is_end ? ++next_itr : next_itr;
    return true;
  }

  size_t tmp_len = 0;
  if (TryReadVariableLengthString(in, next_itr, next_pos, tmp_len)) {
    len += (tmp_len + real_read_size);
    cur_itr = next_itr;
    buf_pos = next_pos;
    return true;
  }
  return false;
}

void Reader::MoveToNextTask() {
  ReadTask *cur, *prv;
  while (ridx_ >= 0) {
    // Readstack is empty,return
    if (ridx_ == 0) {
      ridx_--;
      return;
    }
    cur = &(rstack_[ridx_]);
    prv = &(rstack_[ridx_ - 1]);
    if (cur->idx_ == prv->elements_ - 1) {
      ridx_--;
    } else {
      // Reset type
      assert(cur->idx_ < prv->elements_);
      prv->obj_->u_.array_.push_back(Reply());
      cur->obj_ = &prv->obj_->u_.array_.back();
      cur->elements_ = -1;
      cur->idx_++;
      return;
    }
  }
}

int Reader::ProcessInteger(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                           size_t& pos, ReadTask* cur_task) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  ReadTask* cur = &(rstack_[ridx_]);
  const char* p;
  size_t len = 0;
  p = ReadInteger(in, tmp_itr, tmp_pos, len);
  if (p != nullptr) {
    consume_len += len + 2;
    cur->obj_->Set(IntegerReplyMarker{}, ConvertToInteger(p, len));
    pos = tmp_pos;
    cur_itr = tmp_itr;
    MoveToNextTask();
    return Ret::C_OK;
  }
  return Ret::C_ERR;
}

int Reader::ProcessLineItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                            size_t& pos, ReadTask* cur_task) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  if (ProcessVariableLengthString(in, tmp_itr, consume_len, tmp_pos, cur_task) == Ret::C_OK) {
    pos = tmp_pos;
    cur_itr = tmp_itr;
    MoveToNextTask();
    return Ret::C_OK;
  }
  return Ret::C_ERR;
}

int Reader::ProcessBulkItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                            size_t& pos) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  ReadTask* cur_task = &(rstack_[ridx_]);
  size_t len = 0;
  const char* s = ReadInteger(in, tmp_itr, tmp_pos, len);
  if (s == nullptr) {
    return Ret::C_ERR;
  }
  bool success = false;
  int64_t str_len = ConvertToInteger(s, len);
  if (str_len < 0) {
    cur_task->obj_->Set(NilReplyMarker{});
    success = true;
  } else {
    if (!TryReadable(in, tmp_itr, tmp_pos)) {
      success = false;
    } else {
      success = ProcessFixedLengthString(in, tmp_itr, tmp_pos, str_len, cur_task);
      if (success) {
        consume_len += str_len + 2;
      }
    }
  }
  if (success) {
    consume_len += len + 2;
    pos = tmp_pos;
    cur_itr = tmp_itr;
    MoveToNextTask();
    return Ret::C_OK;
  } else {
    return Ret::C_ERR;
  }
}

int Reader::ProcessMultiBulkItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                                 size_t& consume_len, size_t& pos) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  ReadTask* cur = &(rstack_[ridx_]);
  const char* p;
  int elements;
  // Most deep of rstack_ is 7
  if (ridx_ == 8) {
    protocol_err_ = true;
    cur->obj_->Set(InvalidReplyMarker{});
    return Ret::C_ERR;
  }
  size_t line_len = 0;
  p = ReadInteger(in, tmp_itr, tmp_pos, line_len);
  if (p != nullptr) {
    consume_len += line_len + 2;
    elements = static_cast<int>(ConvertToInteger(p, line_len));
    if (elements == -1) {
      cur->obj_->Set(NilReplyMarker{});
      MoveToNextTask();
    } else {
      cur->obj_->Set(ArrayReplyMarker{});
      // It need update rstack_ when count of elements > 1
      if (elements > 0) {
        cur->elements_ = elements;
        cur->obj_->u_.array_.emplace_back();
        ridx_++;
        rstack_[ridx_].obj_ = &cur->obj_->u_.array_.back();
        rstack_[ridx_].elements_ = -1;
        rstack_[ridx_].idx_ = 0;
        rstack_[ridx_].parent_ = cur;
      } else {
        MoveToNextTask();
      }
    }
    cur_itr = tmp_itr;
    pos = tmp_pos;
    return Ret::C_OK;
  }
  return Ret::C_ERR;
}

int Reader::ProcessItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                        size_t& pos) {
  ReadTask* cur_task = &(rstack_[ridx_]);
  if (cur_task->obj_->type_ == Reply::Type::NONE && !ProcessReplyType(in, cur_itr, consume_len, pos, cur_task)) {
    return Ret::C_ERR;
  }
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  int ret = Ret::C_ERR;

  switch (cur_task->obj_->type_) {
    case Reply::Type::ERROR:
    case Reply::Type::STATUS:
      ret = ProcessLineItem(in, tmp_itr, consume_len, tmp_pos, cur_task);
      break;
    case Reply::Type::INTEGER:
      ret = ProcessInteger(in, tmp_itr, consume_len, tmp_pos, cur_task);
      break;
    case Reply::Type::STRING:
      ret = ProcessBulkItem(in, tmp_itr, consume_len, tmp_pos);
      break;
    case Reply::Type::ARRAY:
      ret = ProcessMultiBulkItem(in, tmp_itr, consume_len, tmp_pos);
      break;
    case Reply::Type::INVALID:
      protocol_err_ = true;
      ret = Ret::C_ERR;
      break;
    default:
      protocol_err_ = true;
      ret = Ret::C_ERR;
      break;
  }
  if (ret != Ret::C_ERR) {
    cur_itr = tmp_itr;
    pos = tmp_pos;
  }
  return ret;
}

int Reader::ProcessVariableLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                                        size_t& consume_len, size_t& pos, ReadTask* cur_task) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  size_t str_len = 0;
  if (!TryReadVariableLengthString(in, tmp_itr, tmp_pos, str_len)) {
    return Ret::C_ERR;
  }
  consume_len += str_len + 2;
  FillStringReply(in, cur_itr, pos, cur_task, str_len);
  cur_itr = tmp_itr;
  pos = tmp_pos;
  return Ret::C_OK;
}

bool Reader::ProcessFixedLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                                      size_t& pos, const size_t fixed_len, ReadTask* cur_task) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  if (!TryReadFixedLengthString(in, tmp_itr, tmp_pos, fixed_len)) {
    return false;
  }
  FillStringReply(in, cur_itr, pos, cur_task, fixed_len);
  cur_itr = tmp_itr;
  pos = tmp_pos;
  return true;
}

bool Reader::GetReply(NoncontiguousBuffer& in, std::deque<std::any>& out, const int pipeline_count) {
  NoncontiguousBuffer::const_iterator parse_itr = in.begin();
  size_t consume_len = 0;
  size_t tmp_pos = 0;

  if (!TryReadable(in, parse_itr, tmp_pos)) {
    return false;
  }

  if (ridx_ == -1) {
    Init();
    assert(reply_.IsNone());
    rstack_[0].elements_ = -1;
    rstack_[0].idx_ = -1;
    rstack_[0].obj_ = &reply_;
    rstack_[0].parent_ = nullptr;
    has_parse_seqno_ = false;
    ridx_ = 0;
    if (pipeline_count > 1) {
      ReadTask* cur = &(rstack_[ridx_]);
      cur->obj_->type_ = Reply::Type::ARRAY;

      cur->obj_->Set(ArrayReplyMarker{});

      cur->elements_ = pipeline_count;
      cur->obj_->u_.array_.emplace_back();
      ridx_++;
      rstack_[ridx_].obj_ = &cur->obj_->u_.array_.back();
      rstack_[ridx_].elements_ = -1;
      rstack_[ridx_].idx_ = 0;
      rstack_[ridx_].parent_ = cur;
    }
  }

  while (ridx_ >= 0 && parse_itr != in.end()) {
    // It has an extra layer compared to a normal cmd when is pipeline
    ReadTask* cur = &(rstack_[ridx_]);
    if (!has_parse_seqno_ || (pipeline_count > 1 && ridx_ == 1 && cur->obj_->type_ == Reply::Type::NONE)) {
      if (ProcessSerialNo(in, parse_itr, consume_len, tmp_pos) <= 0) {
        break;
      } else {
        has_parse_seqno_ = true;
      }
    }
    if (ProcessItem(in, parse_itr, consume_len, tmp_pos) != Ret::C_OK) {
      OutputLog(in, cur, consume_len);
      break;
    }
  }

  if (ridx_ == -1) {
    if (!protocol_err_ && consume_len > 0) {
      in.Skip(consume_len);
    }
    out.emplace_back(std::move(reply_));
    reply_ = Reply();
    return true;
  }

  return false;
}

int Reader::ProcessSerialNo(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                            size_t& pos) {
  NoncontiguousBuffer::const_iterator tmp_itr = cur_itr;
  size_t tmp_pos = pos;
  if (!TryReadable(in, tmp_itr, tmp_pos)) {
    return 0;
  }
  const char* p = tmp_itr->data() + tmp_pos;
  // Do not exist length data
  if (p[0] != '@') {
    pos = tmp_pos;
    cur_itr = tmp_itr;
    return 1;
  }
  tmp_pos++;
  if (!TryReadable(in, tmp_itr, tmp_pos)) {
    return 0;
  }
  size_t len = 0;
  const char* s = ReadInteger(in, tmp_itr, tmp_pos, len);
  if (s != nullptr) {
    consume_len += len + 3;
    ReadTask* cur = &(rstack_[ridx_]);
    cur->obj_->serial_no_ = ConvertToInteger(s, len);
    pos = tmp_pos;
    cur_itr = tmp_itr;
    return 1;
  } else {
    return 0;
  }
}

bool Reader::ProcessReplyType(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                              size_t& consume_len, size_t& pos, ReadTask* cur_task) {
  if (!TryReadable(in, cur_itr, pos)) {
    return false;
  }
  const char* p = cur_itr->data() + pos;
  switch (p[0]) {
    case '-':
      cur_task->obj_->type_ = Reply::Type::ERROR;
      break;
    case '+':
      cur_task->obj_->type_ = Reply::Type::STATUS;
      break;
    case ':':
      cur_task->obj_->type_ = Reply::Type::INTEGER;
      break;
    case '$':
      cur_task->obj_->type_ = Reply::Type::STRING;
      break;
    case '*':
      cur_task->obj_->type_ = Reply::Type::ARRAY;
      break;
    default:
      protocol_err_ = true;
      return false;
  }

  pos += 1;
  consume_len += 1;
  if (!TryReadable(in, cur_itr, pos)) {
    return false;
  }
  return true;
}

void Reader::FillStringReply(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& pos,
                             ReadTask* cur_task, size_t str_len) {
  size_t real_readable_size = cur_itr->size() - pos;
  const char* p = cur_itr->data() + pos;
  if (real_readable_size >= str_len) {
    if (cur_task->obj_->type_ == Reply::Type::ERROR) {
      cur_task->obj_->Set(ErrorReplyMarker{}, p, str_len);
    } else if (cur_task->obj_->type_ == Reply::Type::STATUS) {
      cur_task->obj_->Set(StatusReplyMarker{}, p, str_len);
    } else {
      cur_task->obj_->Set(StringReplyMarker{}, p, str_len);
    }
    return;
  }
  // When span across multiple blocks.
  if (cur_task->obj_->type_ == Reply::Type::ERROR) {
    cur_task->obj_->Set(ErrorReplyMarker{}, p, real_readable_size, str_len);
  } else if (cur_task->obj_->type_ == Reply::Type::STATUS) {
    cur_task->obj_->Set(StatusReplyMarker{}, p, real_readable_size, str_len);
  } else {
    cur_task->obj_->Set(StringReplyMarker{}, p, real_readable_size, str_len);
  }

  NoncontiguousBuffer::const_iterator begin_itr = cur_itr;
  ++begin_itr;
  size_t wait_len = str_len - real_readable_size;

  real_readable_size = begin_itr->size();
  p = begin_itr->data();
  while (wait_len) {
    if (real_readable_size >= wait_len) {
      cur_task->obj_->AppendString(p, wait_len);
      break;
    }
    cur_task->obj_->AppendString(p, real_readable_size);
    wait_len -= real_readable_size;
    ++begin_itr;
    p = begin_itr->data();
    real_readable_size = begin_itr->size();
  }
}

void Reader::OutputLog(NoncontiguousBuffer& in, ReadTask* cur_task, size_t consume_len) {
  if (protocol_err_) {
    NoncontiguousBuffer::const_iterator begin_itr = in.begin();
    while (begin_itr != in.end()) {
      TRPC_LOG_ERROR("Recv size: " << begin_itr->size()
                                   << " ,data: " << std::string(begin_itr->data(), begin_itr->size()));
      ++begin_itr;
    }
    TRPC_LOG_ERROR("consume_len:" << consume_len << ",ridx_" << ridx_ << "cur->obj_->type_:" << cur_task->obj_->type_);
  }
}

}  // namespace redis

}  // namespace trpc
