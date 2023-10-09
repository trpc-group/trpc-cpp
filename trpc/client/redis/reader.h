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

#pragma once

#include <assert.h>

#include <any>
#include <cstring>
#include <deque>
#include <string>

#include "trpc/client/redis/reply.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

namespace redis {

/// @brief redis reader for parsing and reading redis reply
/// @private For internal use purpose only.
class Reader {
 protected:
  enum Ret {
    C_ERR = -1,
    C_OK = 0,
  };

  /// @brief read task warpper
  /// @private For internal use purpose only.
  struct ReadTask {
    // count of element
    int elements_{0};

    // index of parent_ (array)
    int idx_{-1};

    //  reply of read task
    Reply* obj_{nullptr};

    // parent read task
    struct ReadTask* parent_{nullptr};
  };

 public:
  /// @private For internal use purpose only.
  Reader() = default;

  /// @private For internal use purpose only.
  ~Reader() = default;

  /// @brief Whether protocol error
  /// @private For internal use purpose only.
  bool IsProtocolError() { return protocol_err_; }

  /// @brief whether need cache.
  /// It indicates parse protocol right and data not ready when ridx_ != -1 and protocol_err_ == false,so it need
  /// cache;
  /// @private For internal use purpose only.
  bool NeedCache() { return ridx_ != -1 && protocol_err_ == false; }

  /// @brief Get redis reply
  /// @private For internal use purpose only.
  bool GetReply(NoncontiguousBuffer& in, std::deque<std::any>& out, const int pipeline_count = 1);

 protected:
  /// @private For internal use purpose only.
  void Init();

  /// @brief whether can be read.
  /// @private For internal use purpose only.
  bool TryReadable(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& pos);

  /// @brief Whether noncontiguousBuffer left data contains full fix-length string
  /// For example: "trpc\r\n" ,delimiter "\r""\n" may span in more than one block:
  /// 1）"trpc\r\n"
  /// 2）"trpc\r" 、"\n"
  /// 3) "trpc"、 "\r\n"
  /// 4) "trpc"、 "\r" 、"\n"
  /// @private For internal use purpose only.
  bool TryReadFixedLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& buf_pos,
                                size_t fixed_len);

  /// @private For internal use purpose only.
  bool TryReadVariableLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                                   size_t& buf_pos, size_t& len);

  /// @private For internal use purpose only.
  void MoveToNextTask();

  /// @private For internal use purpose only.
  bool ProcessFixedLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& pos,
                                const size_t fixed_len, ReadTask* cur);

  /// @private For internal use purpose only.
  bool ProcessReplyType(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                        size_t& pos, ReadTask* cur_task);

  /// @private For internal use purpose only.
  int ProcessInteger(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                     size_t& pos, ReadTask* cur_task);

  /// @private For internal use purpose only.
  int ProcessVariableLengthString(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr,
                                  size_t& consume_len, size_t& pos, ReadTask* cur_task);

  /// @brief Seek to "\r\n"
  /// @private For internal use purpose only.
  const char* SeekNewLine(const char* s, int len);

  /// @private For internal use purpose only.
  int64_t ConvertToInteger(const char* s, size_t len);

  /// @private For internal use purpose only.
  const char* ReadInteger(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& buf_pos,
                          size_t& len);

  /// @brief parse one line data
  /// @private For internal use purpose only.
  int ProcessLineItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                      size_t& pos, ReadTask* cur_task);

  /// @brief parse bulk item
  /// @private For internal use purpose only.
  int ProcessBulkItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                      size_t& pos);

  /// @brief parse multiple bulk item
  /// @private For internal use purpose only.
  int ProcessMultiBulkItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                           size_t& pos);

  /// @brief parse item
  /// @private For internal use purpose only.
  int ProcessItem(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                  size_t& pos);

  /// @brief parse serial number
  /// @private For internal use purpose only.
  int ProcessSerialNo(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& consume_len,
                      size_t& pos);

  /// @brief Fill String(error/status) type of reply;
  /// @private For internal use purpose only.
  void FillStringReply(NoncontiguousBuffer& in, NoncontiguousBuffer::const_iterator& cur_itr, size_t& pos,
                       ReadTask* cur_task, size_t str_len);

  /// @brief Output NoncontiguousBuffer to log
  /// @private For internal use purpose only.
  void OutputLog(NoncontiguousBuffer& in, ReadTask* cur_task, size_t consume_len);

 protected:
  // length of data
  char r_buf_[24] = {0};

  // Whether protocol parse error
  bool protocol_err_{false};

  // Stack of readtask
  ReadTask rstack_[9];

  // Index of readtask
  int ridx_{-1};

  // Redis reply for temp usage
  Reply reply_;

  // Whether has parsed seq number
  bool has_parse_seqno_{false};
};

}  // namespace redis

}  // namespace trpc
