// trpc/util/http/sse/sse_event.h
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

#include <string>
#include <optional>
#include <cstdint>

namespace trpc::http::sse {

/// @brief Represents a Server-Sent Event (SSE) message.
/// @note SSE format: https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events
class SseEvent {
 public:
  SseEvent() = default;
  
  /// @brief Constructor with all fields
  SseEvent(std::string event_type, std::string data, std::optional<std::string> id = std::nullopt, 
           std::optional<uint32_t> retry = std::nullopt)
      : event_type_(std::move(event_type)), data_(std::move(data)), id_(std::move(id)), retry_(retry) {}

  /// @brief Get the event type
  const std::string& GetEventType() const { return event_type_; }
  
  /// @brief Set the event type
  void SetEventType(std::string event_type) { event_type_ = std::move(event_type); }

  /// @brief Get the data content
  const std::string& GetData() const { return data_; }
  
  /// @brief Set the data content
  void SetData(std::string data) { data_ = std::move(data); }

  /// @brief Get the event ID
  const std::optional<std::string>& GetId() const { return id_; }
  
  /// @brief Set the event ID
  void SetId(std::optional<std::string> id) { id_ = std::move(id); }

  /// @brief Get the retry timeout in milliseconds
  const std::optional<uint32_t>& GetRetry() const { return retry_; }
  
  /// @brief Set the retry timeout in milliseconds
  void SetRetry(std::optional<uint32_t> retry) { retry_ = retry; }

 private:
  std::string event_type_;                    ///< Event type (optional)
  std::string data_;                          ///< Event data
  std::optional<std::string> id_;             ///< Event ID (optional)
  std::optional<uint32_t> retry_;             ///< Retry timeout in milliseconds (optional)
};

}  // namespace trpc::http::sse