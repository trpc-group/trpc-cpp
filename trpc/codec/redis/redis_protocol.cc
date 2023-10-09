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

#include "trpc/codec/redis/redis_protocol.h"

#include <algorithm>
#include <vector>

namespace trpc {

bool RedisRequestProtocol::ZeroCopyEncode(NoncontiguousBuffer& buff) {
  // Redis command has already been encoded once and can be directly copied and used when redis_req->do_RESP_ == false
  if (!redis_req.do_RESP_) {
    auto itr = redis_req.params_.begin();
    size_t cmd_size = itr->size();
    NoncontiguousBufferBuilder builder;
    builder.Append(itr->c_str(), cmd_size);
    buff = builder.DestructiveGet();
    return true;
  }
  NoncontiguousBufferBuilder builder;
  return EncodeImpl(builder, buff);
}

bool RedisRequestProtocol::EncodeImpl(NoncontiguousBufferBuilder& builder, NoncontiguousBuffer& buff) const {
  size_t buff_size = 0;
  char tmp_buff[16] = {0};
  snprintf(tmp_buff, sizeof(tmp_buff), "*%u\r\n", static_cast<uint32_t>(redis_req.params_.size()));
  buff_size = strlen(tmp_buff);
  builder.Append(tmp_buff, buff_size);
  for (auto& r : redis_req.params_) {
    snprintf(tmp_buff, sizeof(tmp_buff), "$%u\r\n", static_cast<uint32_t>(r.size()));
    buff_size = strlen(tmp_buff);
    builder.Append(tmp_buff, buff_size);
    builder.Append(r.c_str(), r.size());
    builder.Append("\r\n", 2);
  }
  buff = builder.DestructiveGet();
  return true;
}

}  // namespace trpc
