## Filter
The filter design follows the following principles:
1. Filter points need to be paired.
The filter implementation is based on tracking point mechanism. Currently, both the client and server support multiple 
pairs of matching filter points, such as pre-RPC and post-RPC call, pre-network and post-network packet transmission, 
etc.

2. Support execute filters in configuration order.
The framework supports global filters and service-level filters. Global filters take precedence over service-level 
filters and are executed in the order specified in the configuration file. Specifically:
1) When executing the pre filter point, the global filter is executed first in order, followed by the service-level 
   filter in order.
2) When executing the post filter point, the order is reversed from the pre filter point filter to ensure that the 
   filter that is executed later exits first.
3. Support for interrupting and exiting filter execution If the filter is rejected during execution. Specifically: 
1) If the pre filter point is rejected during execution, the corresponding post filter point will exit from the last 
   successful filter executed before the rejection point.
2) At the same time, the execution status of the post filter point is not checked, to ensure that the corresponding  
   post filter point can be executed normally and complete the loop after the pre filter point is executed successfully.

## FilterManager
The FilterManager class is a singleton class used for managing filters, and it provides the following features:
1. As a filter factory, it provides interfaces for registering and obtaining filters based on filter name.
  ```cpp
  // Put the server filter into the server filter collection based on the filter name.
  bool AddMessageServerFilter(const MessageServerFilterPtr& filter);

  // Get the server filter by name.
  MessageServerFilterPtr GetMessageServerFilter(const std::string& name);

  // Put the client filter into the client filter collection based on the filter name.
  bool AddMessageClientFilter(const MessageClientFilterPtr& filter);

  // Get the client filter by name.
  MessageClientFilterPtr GetMessageClientFilter(const std::string& name);
  ```
2. Provides an interface for registering global filters. When registering global filters, it will categorize the fiters 
   based on the filter point and store them in order for later execution based on the filter point.
  ```cpp
  // Add a global server filter.
  bool AddMessageServerGlobalFilter(const MessageServerFilterPtr& filter);

  // Get global server filters by filter point.
  const std::deque<MessageServerFilterPtr>& GetMessageServerGlobalFilter(FilterPoint type);

  // Add a global client filter.
  bool AddMessageClientGlobalFilter(const MessageClientFilterPtr& filter);

  // Get global client filters by filter point.
  const std::deque<MessageClientFilterPtr>& GetMessageClientGlobalFilter(FilterPoint type);
  ```

## FilterController
The FilterController is used to execute filters, and it provides the following features:
1. Provides the ability to run filters in specified fitler point. The interface is as follows:
  ```cpp
  // Run server filters in specified fitler point.
  FilterStatus RunMessageServerFilters(FilterPoint type, Args&&... args);

  // Run client filters in specified fitler point.
  FilterStatus RunMessageClientFilters(FilterPoint type, Args&&... args);
  ```
2. Provides interfaces for adding and executing service-level filters.
   Note: For the client, a ServiceProxy has a FilterController responsible for executing client filters. 
   For the server, a Service has a FilterController responsible for executing server filters. The execution order is 
   as follows: first, execute global filters (extracted from the FilterManager), then execute service-level filters 
   (found in the FilterManager based on the filter name). When creating a ServiceProxy or Service, the framework uses 
   the following two interfaces to place the service filters to be executed in the FilterController object according 
   to the configuration order.
  ```cpp
  // Add a service-level's server filter.
  bool AddMessageServerFilter(const MessageServerFilterPtr& filter);

  // Add a service-level's client filter.
  bool AddMessageClientFilter(const MessageClientFilterPtr& filter);
  ```
