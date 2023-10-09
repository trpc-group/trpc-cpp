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

#include <string.h>

#include <any>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/admin/base_funcs.h"

namespace trpc::admin {

class MonitorMetrics;

template <typename T, typename Op>
class PassiveMetrics;

class MonitorVarsMap {
 public:
  static MonitorVarsMap* GetInstance() {
    static MonitorVarsMap instance;
    return &instance;
  }

  ~MonitorVarsMap() { vars_.clear(); }

  void AddToVarsMap(const std::string& name, MonitorMetrics* p_metric) {
    std::lock_guard<std::mutex> lock(mutex_);
    vars_[name] = p_metric;
  }

  void DelFromVarsMap(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = vars_.find(name);
    if (it != vars_.end()) {
      vars_.erase(it);
    }
  }

  MonitorMetrics* GetMonitorMetrics(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = vars_.find(name);
    if (it != vars_.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }

 private:
  MonitorVarsMap() = default;

  MonitorVarsMap(const MonitorVarsMap&) = delete;
  MonitorVarsMap& operator=(const MonitorVarsMap&) = delete;

 private:
  std::mutex mutex_;
  std::map<std::string, MonitorMetrics*> vars_;
};

enum class DisplayStatus { kExpose = 0, kHide = 1 };

enum class OpTypes { kMax = 0, kMin = 1, kCount = 2, kSum = 3, kAverage = 4 };

template <typename Tp>
class BasicOperation {
 public:
  BasicOperation() {}
  virtual ~BasicOperation() {}
  virtual Tp DoArithmetic(const Tp& lhs, const Tp& rhs) = 0;
  virtual Tp DivideOnAddition(const Tp& res, int count) { return res; }
};

template <typename Tp>
class MaxOperation : public BasicOperation<Tp> {
 public:
  Tp DoArithmetic(const Tp& lhs, const Tp& rhs) override { return lhs >= rhs ? lhs : rhs; }
};

template <typename Tp>
class MinOperation : public BasicOperation<Tp> {
 public:
  Tp DoArithmetic(const Tp& lhs, const Tp& rhs) override { return lhs < rhs ? lhs : rhs; }
};

template <typename Tp>
class AddOperation : public BasicOperation<Tp> {
 public:
  Tp DoArithmetic(const Tp& lhs, const Tp& rhs) override { return lhs + rhs; }
};

template <typename Tp>
class AverageOperation : public BasicOperation<Tp> {
 public:
  Tp DoArithmetic(const Tp& lhs, const Tp& rhs) override { return lhs + rhs; }
  Tp DivideOnAddition(const Tp& res, int count) override { return res / count; }
};

template <typename T>
class SeriesSamples {
 public:
  explicit SeriesSamples(BasicOperation<T>* op) : op_(op), nsecond_(0), nminute_(0), nhour_(0), nday_(0) {}

  ~SeriesSamples() { delete op_; }

  void Append(const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return AppendSecond(value, op_);
  }

  void GetSeriesData(rapidjson::Value& result, rapidjson::Document::AllocatorType& alloc) {
    result.AddMember("label", "trend", alloc);

    rapidjson::Value data_arr(rapidjson::kArrayType);
    int len = 60 + 60 + 24 + 30;
    for (int i = 0; i < len; i++) {
      rapidjson::Value item(rapidjson::kArrayType);
      item.PushBack(i, alloc);
      item.PushBack(data_.array_[len - 1 - i], alloc);
      data_arr.PushBack(item, alloc);
    }
    result.AddMember("data", data_arr, alloc);
  }

  void AppendSecond(const T& value, BasicOperation<T>* op);
  void AppendMinute(const T& value, BasicOperation<T>* op);
  void AppendHour(const T& value, BasicOperation<T>* op);
  void AppendDay(const T& value);

 private:
  struct Data {
   public:
    Data() { memset(array_, 0, sizeof(array_)); }

    T& Second(int index) { return array_[index]; }
    const T& Second(int index) const { return array_[index]; }

    T& Minute(int index) { return array_[60 + index]; }
    const T& Minute(int index) const { return array_[60 + index]; }

    T& Hour(int index) { return array_[120 + index]; }
    const T& Hour(int index) const { return array_[120 + index]; }

    T& Day(int index) { return array_[144 + index]; }
    const T& Day(int index) const { return array_[144 + index]; }

    T array_[60 + 60 + 24 + 30];
  };

 protected:
  BasicOperation<T>* op_;
  std::mutex mutex_;
  char nsecond_;
  char nminute_;
  char nhour_;
  char nday_;
  Data data_;
};

template <typename T>
void SeriesSamples<T>::AppendSecond(const T& value, BasicOperation<T>* op) {
  data_.Second(nsecond_) = value;
  ++nsecond_;
  if (nsecond_ >= 60) {
    nsecond_ = 0;
    T res = data_.Second(0);
    for (int i = 1; i < 60; ++i) {
      res = op_->DoArithmetic(res, data_.Second(i));
    }
    res = op->DivideOnAddition(res, 60);
    AppendMinute(res, op);
  }
}

template <typename T>
void SeriesSamples<T>::AppendMinute(const T& value, BasicOperation<T>* op) {
  data_.Minute(nminute_) = value;
  ++nminute_;
  if (nminute_ >= 60) {
    nminute_ = 0;
    T res = data_.Minute(0);
    for (int i = 1; i < 60; ++i) {
      res = op_->DoArithmetic(res, data_.Minute(i));
    }
    res = op->DivideOnAddition(res, 60);
    AppendHour(res, op);
  }
}

template <typename T>
void SeriesSamples<T>::AppendHour(const T& value, BasicOperation<T>* op) {
  data_.Hour(nhour_) = value;
  ++nhour_;
  if (nhour_ >= 24) {
    nhour_ = 0;
    T res = data_.Hour(0);
    for (int i = 1; i < 24; ++i) {
      res = op_->DoArithmetic(res, data_.Hour(i));
    }
    res = op->DivideOnAddition(res, 24);
    AppendDay(res);
  }
}

template <typename T>
void SeriesSamples<T>::AppendDay(const T& value) {
  data_.Day(nday_) = value;
  ++nday_;
  if (nday_ >= 30) {
    nday_ = 0;
  }
}

class MonitorMetrics {
 public:
  explicit MonitorMetrics(const std::string& name) {
    name_ = name;
    Expose();
  }

  virtual ~MonitorMetrics() {}

  virtual void TakeSample(std::any value) = 0;

  virtual void GetHistoryData(rapidjson::Value& result, rapidjson::Document::AllocatorType& alloc) = 0;

  void Expose() {
    ExposeImpl();
    curr_state_ = DisplayStatus::kExpose;
  }

  virtual void ExposeImpl() { MonitorVarsMap::GetInstance()->AddToVarsMap(name_, this); }

  void Hide() {
    MonitorVarsMap::GetInstance()->DelFromVarsMap(name_);
    curr_state_ = DisplayStatus::kHide;
  }

  const std::string& GetName() { return name_; }

  // private:
  DisplayStatus curr_state_;
  std::string name_;
};

template <typename T, typename Op>
class PassiveMetrics : public MonitorMetrics {
 public:
  PassiveMetrics(const std::string& name, Op* op) : MonitorMetrics(name) {
    series_ = std::make_shared<SeriesSamples<T>>(op);
  }

  ~PassiveMetrics() override = default;

  void TakeSample(std::any value) override {
    T nvalue = std::any_cast<T>(value);
    series_->Append(nvalue);
  }

  void GetHistoryData(rapidjson::Value& result, rapidjson::Document::AllocatorType& alloc) override {
    return series_->GetSeriesData(result, alloc);
  }

 private:
  std::shared_ptr<SeriesSamples<T>> series_;
};

void InitSystemVars();
void UpdateSystemVars();

void GetVarSeriesData(const std::string var_name, rapidjson::Value& result, rapidjson::Document::AllocatorType& alloc);

}  // namespace trpc::admin
