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

#include "trpc/util/buffer/contiguous_buffer.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

namespace trpc {

ContiguousBuffer::ContiguousBuffer(size_t initial_size) : mem_size_(initial_size) {
  if (initial_size > 0) {
    mem_ptr_ = new char[initial_size];
  }
}

ContiguousBuffer::ContiguousBuffer(char* mem_ptr, size_t size) : mem_size_(size), write_pos_(size), mem_ptr_(mem_ptr) {}

ContiguousBuffer::ContiguousBuffer(ContiguousBuffer&& other) {
  read_pos_ = std::exchange(other.read_pos_, 0);
  write_pos_ = std::exchange(other.write_pos_, 0);
  mem_size_ = std::exchange(other.mem_size_, 0);
  mem_ptr_ = std::exchange(other.mem_ptr_, nullptr);
}

ContiguousBuffer& ContiguousBuffer::operator=(ContiguousBuffer&& other) {
  if (this != &other) {
    read_pos_ = std::exchange(other.read_pos_, 0);
    write_pos_ = std::exchange(other.write_pos_, 0);
    mem_size_ = std::exchange(other.mem_size_, 0);
    mem_ptr_ = std::exchange(other.mem_ptr_, nullptr);
  }
  return *this;
}

ContiguousBuffer::~ContiguousBuffer() {
  if (mem_ptr_) {
    delete[] mem_ptr_;
    mem_ptr_ = nullptr;
  }
}

void ContiguousBuffer::Resize(size_t size) noexcept {
  if (mem_size_ >= size) {
    mem_size_ = size;
  } else {
    if (mem_ptr_) {
      delete[] mem_ptr_;
      mem_ptr_ = nullptr;
    }
    mem_ptr_ = new char[size];
    mem_size_ = size;
  }

  read_pos_ = 0;
  write_pos_ = 0;
}

void ContiguousBuffer::DestructiveReset() {
  read_pos_ = 0;
  write_pos_ = 0;
  mem_size_ = 0;
  mem_ptr_ = nullptr;
}

void ContiguousBuffer::Reset() {
  read_pos_ = 0;
  write_pos_ = 0;
  mem_size_ = 0;

  if (mem_ptr_) {
    delete[] mem_ptr_;
    mem_ptr_ = nullptr;
  }
}

}  // namespace trpc
