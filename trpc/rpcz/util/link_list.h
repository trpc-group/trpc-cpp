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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

namespace trpc::rpcz {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/butil/containers/linked_list.h

/// @brief Node of double-linked list.
/// @private
template <typename T>
class LinkNode {
 public:
  /// @brief Default constructor.
  LinkNode() : previous_(this), next_(this) {}

  /// @brief Constructor to set neighbour nodes.
  LinkNode(LinkNode<T>* previous, LinkNode<T>* next) : previous_(previous), next_(next) {}

  /// @brief Insert current node into a list before node e.
  void InsertBefore(LinkNode<T>* e) {
    this->next_ = e;
    this->previous_ = e->previous_;
    e->previous_->next_ = this;
    e->previous_ = this;
  }

  /// @brief Current node is already inside list, merge current list with the list containing node e.
  ///        After merging, current node is before node e.
  void InsertBeforeAsList(LinkNode<T>* e) {
    LinkNode<T>* prev = this->previous_;
    prev->next_ = e;
    this->previous_ = e->previous_;
    e->previous_->next_ = this;
    e->previous_ = prev;
  }

  /// @brief Insert current node into a list after node e.
  void InsertAfter(LinkNode<T>* e) {
    this->next_ = e->next_;
    this->previous_ = e;
    e->next_->previous_ = this;
    e->next_ = this;
  }

  /// @brief Current node is already inside list, merge current list with the list containing node e.
  ///        After merging, current node is after node e.
  void InsertAfterAsList(LinkNode<T>* e) {
    LinkNode<T>* prev = this->previous_;
    prev->next_ = e->next_;
    this->previous_ = e;
    e->next_->previous_ = prev;
    e->next_ = this;
  }

  /// @brief Remove current node from list containing it.
  void RemoveFromList() {
    this->previous_->next_ = this->next_;
    this->next_->previous_ = this->previous_;
    this->next_ = this;
    this->previous_ = this;
  }

  /// @brief Get previous node.
  LinkNode<T>* previous() const { return previous_; }

  /// @brief Get next node.
  LinkNode<T>* next() const { return next_; }

  /// @brief Return value ptr which contains current node as first element.
  const T* value() const { return static_cast<const T*>(this); }

  /// @brief Same as above.
  T* value() { return static_cast<T*>(this); }

  /// @brief Disable Copy and Assignment.
  LinkNode(const LinkNode&) = delete;
  LinkNode& operator=(const LinkNode&) = delete;

 private:
  // Previous node.
  LinkNode<T>* previous_;
  // Next node.
  LinkNode<T>* next_;
};

/// @brief Double-linked list.
/// @private
template <typename T>
class LinkedList {
 public:
  /// @brief Default constructot.
  LinkedList() = default;

  /// @brief Append node into list before root node.
  void Append(LinkNode<T>* e) { e->InsertBefore(&root_); }

  /// @brief Get head node of list.
  LinkNode<T>* head() const { return root_.next(); }

  /// @brief Get tail node of list.
  LinkNode<T>* tail() const { return root_.previous(); }

  /// @brief Get root node of list.
  const LinkNode<T>* end() const { return &root_; }

  /// @brief Check list is empty or not.
  bool empty() const { return head() == end(); }

  /// @brief Disable Copy and Assignment.
  LinkedList(const LinkedList&) = delete;
  LinkedList& operator=(const LinkedList&) = delete;

 private:
  // Root node.
  LinkNode<T> root_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::rpcz
#endif
