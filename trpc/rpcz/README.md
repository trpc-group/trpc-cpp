This directory contains the implementation of tRPC-Cpp rpcz.
# Design
- As a write-mostly scenario, we use thread local mechanism to improve performance of rpcz, every time user request coming, span info is written into thread local blcok for further merging.
- Span merging is done inside isolated thread named periphery_task_scheduler, which periodically merge thread local spans into global map, which user can query through admin service.
- To prevent oom, a high low water level sampler is implemented to let user configure the frequency of sampling, user can also set their sampling function to control what to sample.
- To prevent oom, user can also configure how long the spans can be stored before removed from memory.

# Usage
## Build
Build with bazel
```
build --define trpc_include_rpcz=true
```

Build with cmake
```
cmake .. -DTRPC_BUILD_WITH_RPCZ=ON
```

## Global conf
```
global:
	rpcz:
		lower_water_level: 500
		high_water_level: 1000
		sample_rate: 50
		store_time_min: 10
		collect_interval_ms: 500
		remove_interval_ms: 5000
		print_spans_num: 10
```

## Filter conf
Open server filter
```
server:
	filter:
		- rpcz
```
Open client filter
```
client:
	filter:
		- rpcz
```
Note: in route scenario, both client and server filter should be configured.

# Attentions
- TRPC_RPCZ_PRINT macro can be used to find out the bottleneck inside user callback funtion, which contains user logic not related to rpc framewok.
- There is delay between span is created and being seen by user through admin service, as periphery_task_scheduler thread is merging spans periodically.
