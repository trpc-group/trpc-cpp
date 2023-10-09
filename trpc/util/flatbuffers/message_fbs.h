//
// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
// Copy and modify from https://github.com/google/flatbuffers/blob/v23.5.26/include/flatbuffers/grpc.h.

#pragma once

namespace flatbuffers {
namespace trpc {

/// @brief  The base class of message used by flat-buffers.
class MessageFbs {
 public:
  inline MessageFbs() {}
  virtual ~MessageFbs() {}
  MessageFbs(const MessageFbs& other) = delete;
  MessageFbs& operator=(const MessageFbs& other) = delete;
  MessageFbs(MessageFbs&& other) = delete;
  MessageFbs& operator=(MessageFbs&& other) = delete;

  /// @brief Gets a pointer to the data.
  virtual const uint8_t* mutable_data() const = 0;
  virtual const uint8_t* data() const = 0;

  /// @brief Gets the length of binary data.
  virtual uint32_t ByteSizeLong() const = 0;
  // For compatibility.
  virtual size_t size() const = 0;

  /// @brief Serializes the message object to binary data.
  virtual bool SerializeToArray(char* arr, uint32_t len) const = 0;

  /// @brief Deserialize binary data into a message object.
  virtual bool ParseFromArray(const char* arr, uint32_t len) = 0;
};

}  // namespace trpc
}  // namespace flatbuffers
