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

#include "trpc/util/http/content.h"

namespace trpc::http {

ssize_t ContentProvider::Read(uint8_t* data, std::size_t len) {
  auto content_buffer_size = content_buffer_.ByteSize();
  // Returns EOF when content buffer is empty.
  if (content_buffer_size == 0) {
    return 0;
  }

  size_t expect_bytes = std::min(len, content_buffer_size);

  // Try to read the first block.
  if (content_buffer_.FirstContiguous().size() >= expect_bytes) {
    memcpy(data, content_buffer_.FirstContiguous().data(), expect_bytes);
  } else {
    FlattenToSlow(content_buffer_, static_cast<void*>(data), expect_bytes);
  }

  content_buffer_.Skip(expect_bytes);
  return expect_bytes;
}

ssize_t ContentProvider::Write(const uint8_t* data, std::size_t len) {
  // Got EOF
  if (len == 0) {
    return 0;
  }

  NoncontiguousBufferBuilder buffer_builder{};
  buffer_builder.Append(data, len);
  content_buffer_.Append(buffer_builder.DestructiveGet());
  return len;
}

std::string ContentProvider::SerializeToString() const {
  std::string ss{""};
  if (!content_) {
    for (const auto& item : content_buffer_) {
      ss += std::string_view{item.data(), item.size()};
    }
  } else if (!content_->empty()) {
    ss += content_.value();
  }
  return ss;
}

void ContentProvider::SerializeToString(NoncontiguousBuffer& buff) const& { SerializeToStringImpl(*this, buff); }
void ContentProvider::SerializeToString(NoncontiguousBuffer& buff) && { SerializeToStringImpl(std::move(*this), buff); }

template <typename T>
void ContentProvider::SerializeToStringImpl(T&& self, NoncontiguousBuffer& buff) {
  NoncontiguousBufferBuilder builder;
  if (!self.content_) {
    builder.Append(std::forward<T>(self).content_buffer_);
  } else if (!self.content_->empty()) {
    builder.Append(self.content_.value());
  }
  buff.Append(builder.DestructiveGet());
}

}  // namespace trpc::http
