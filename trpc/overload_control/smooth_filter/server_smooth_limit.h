//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once

#include "trpc/overload_control/smooth_filter/server_request_roll_queue.h"
#include "trpc/overload_control/smooth_filter/server_trick_time.h"
#include "trpc/overload_control/smooth_filter/server_overload_controller.h"

#include<string>
#include <cstdint>

#include "trpc/util/function.h"

/// @brief The first step is to implement a time queue that stores the number of requests
/// @note We use a queue to represent a time window of 1s, and the number of queues N represents N time slots
/// Optimization 1: We suggest that the size of N be 2 ^ n-2, so that frequent redundancy operations can be replaced by AND operations. 
/// We have considered directly expanding the user's memory to 2 ^ n-2, but in reality, the expansion may take up a large amount of memory, so this is just a suggestion
/// Optimization 2: We add an atomic variable to represent the total number of smooth windows, which can save the need to calculate for loops every time (which involves concurrency control)
/// The queue itself uses atomic to represent some intra class elements, ensuring that the atomicity of individual variable modification operations is not strictly synchronized
/// But when calling the checkpoint() of this queue, a double loop is used to ensure that there will not be a moment when the threshold is exceeded

/// The second step of the plugin requires a timer to process the forward movement of the window at a scheduled time
/// The timer here needs to open a thread to do so, so it requires interfaces for creating, starting, and destroying threads

/// 1. Implement a circular queue using vector<uint64_t>of size N
/// 2. The size of the circular queue represents the number of time slots divided into 1s
/// 3. Each time slot corresponds to an integer element in the queue, storing the cumulative number of requests in that time slot
/// 4. The passage of time corresponds to the movement of the 'smooth window'
/// Every time the timer is triggered (once every 1/N second), the oldest time slot is removed and a new time slot is added (the count is reset to 0)
/// 5. The number of requests in the current time slot accumulates to the count of the latest time slot
/// 6. Accumulate the count within the current queue range when calculating the total number of requests within 1s
/// 7. The rotation of the time slot is achieved by calling the NextFrame() function through an external timer thread

namespace trpc::overload_control 
{
/// @var: Default number of time frames per second
constexpr int32_t kDefaultNum = 100;

class SmoothLimit : public ServerOverloadController
{
public:
  /// @brief Initialize the sliding window current limiting plugin
  /// @param Name of plugin Current limit quantity Record monitoring logs or not Number of time slots
  explicit SmoothLimit(std::string name,int64_t limit, bool is_report = false, int32_t window_size = kDefaultNum);

  /// @note Do nothing, the entire plugin requires manual destruction of only thread resources
  /// @brief From the implementation of filter, it can be seen that before destruction
  ///the plugin's stop() and destroy() will be called first
  ~SmoothLimit();

  /// @note Func called before onrequest() is called at the buried point location, and checkpoint() is called internally
  /// @param Context represents the storage of status within the context service name、caller name
  /// @param By matching the factory object (singleton) with the name information, specific plugin strategies can be found
  bool BeforeSchedule(const ServerContextPtr& context);

  /// @note There is no need to call funcs for burying points here
  bool AfterSchedule(const ServerContextPtr& context){return true;}

  std::string Name() {return name_;}
  
  ///@brief Initialize thread resources
  bool Init();

  /// @brief End the 'loop' of the scheduled thread while making it 'joinable'
  void Stop();

  /// @brief Destroy thread
  void Destroy() ;
public: //This place should be private. I tested it in the test, so it was defined as public
  ///@brief Check if it exceeds the current limit of the window
  bool CheckLimit(const ServerContextPtr& check_param);

  /// @brief Retrieve the number of requests occurring within the current window
  int64_t GetCurrCounter() ;

  /// @brief Obtain the current limiting threshold
  int64_t GetMaxCounter() const  { return limit_; }
public:
  //Name of plugin
  std::string name_;

  //Regularly call this func to move the smooth window
  void OnNextTimeFrame();

  //Number of requests limited by smooth window
  int64_t limit_;

  //Whether to record monitoring data
  bool is_report_{false};

  //The number of time slots in the smooth window
  int32_t window_size_{kDefaultNum};

  //Store the smooth window time slot for requests
  RequestRollQueue requestrollque_;

  //Timer starts, thread calls NextFrame() every 1/N second to move the smooth window
  TickTime tick_timer_;

};

}
#endif

