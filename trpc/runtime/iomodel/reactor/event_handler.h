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

#include <cstdint>
#include <memory>

#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Base class for reactor network-io event handler
class alignas(8) EventHandler : public trpc::RefCounted<EventHandler> {
 public:
  /// @brief Event type
  enum EventType : uint8_t {
    kReadEvent = 0x01,
    kWriteEvent = 0x02,
    kCloseEvent = 0x04,
    kMaxEvent = 0xff,
  };

  /// @brief Event handler state
  enum EventHandlerState : uint8_t {
    kCreate = 0,
    kMod = 1,
  };

  /// @brief The initialization state of event handler
  enum InitializationState : uint8_t {
    kUnInitialized = 0,
    kInitializedSuccess = 1,
  };

 public:
  virtual ~EventHandler() = default;

  /// @brief Get the fd of eventhandler
  int GetFd() const { return fd_; }

  /// @brief Set the fd of eventhandler
  void SetFd(int fd) { fd_ = fd; }

  /// @brief Get the initialization state of event handler
  uint8_t GetInitializationState() const { return initialization_state_; }

  /// @brief Set the initialization state of event handler
  void SetInitializationState(uint8_t initialization_state) {
    initialization_state_ = initialization_state;
  }

  /// @brief Get the set event that will be registered to Poller
  uint8_t GetSetEvents() const { return set_events_; }

  /// @brief Set the set event that will be registered to Poller
  void SetSetEvents(uint8_t events) { set_events_ = events; }

  /// @brief Check whether the event of interest is set
  bool HasSetEvent() const { return set_events_ != 0; }

  /// @brief Get the recv event that will be triggered by Poller
  uint8_t GetRecvEvents() const { return recv_events_; }

  /// @brief Set the recv event that will be triggered by Poller
  void SetRecvEvents(uint8_t events) { recv_events_ = events; }

  /// @brief Set the state of eventhandler
  void SetState(uint8_t state) { state_ = state; }

  /// @brief Get the state of eventhandler
  uint8_t GetState() const { return state_; }

  /// @brief Get the event data that will be registered to Poller
  uint64_t GetEventData() { return reinterpret_cast<uint64_t>(this); }

  /// @brief Enable the events this eventhander concerned
  void EnableEvent(uint8_t events);

  /// @brief Disbale the events this eventhander concerned
  void DisableEvent(uint8_t events);

  /// @brief Disbale all events this eventhander concerned
  void DisableAllEvent();

  /// @brief The eventhandler execute function after receiving the event
  virtual void HandleEvent();

 protected:
  virtual int HandleReadEvent() { return 0; }

  virtual int HandleWriteEvent() { return 0; }

  virtual void HandleCloseEvent() {}

 private:
  // Eventhandler fd
  int fd_ = 0;

  // The initialization state of event handler
  uint8_t initialization_state_{InitializationState::kUnInitialized};

  // The state of event_handler
  uint8_t state_{EventHandlerState::kCreate};

  // The set event that will be registered to Poller
  uint8_t set_events_{0};

  // The recv event that will be triggered by Poller
  uint8_t recv_events_{0};
};

using EventHandlerPtr = RefPtr<EventHandler>;

}  // namespace trpc
