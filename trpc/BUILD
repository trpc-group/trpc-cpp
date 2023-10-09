licenses(["notice"])

package(default_visibility = ["//visibility:public"])

# Some objects used in the internal main process of the framework use object pool types, and the default
# implementation is global.
config_setting(
    name = "trpc_shared_nothing_objectpool",
    values = {"define": "trpc_shared_nothing_objectpool=true"},
)

# Whether to turn off the object pool, it is turned on by default.
config_setting(
    name = "trpc_disabled_objectpool",
    values = {"define": "trpc_disabled_objectpool=true"},
)

# The internal memory pool type of the framework is the global implementation by default.
config_setting(
    name = "trpc_shared_nothing_mem_pool",
    values = {"define": "trpc_shared_nothing_mem_pool=true"},
)

# Whether to disable the mem_pool, it is not disabled by default.
config_setting(
    name = "trpc_disabled_mem_pool",
    values = {"define": "trpc_disabled_mem_pool=true"},
)

# Enable protobuf arena optimization, which is turned off by default in the framework.
config_setting(
    name = "trpc_proto_use_arena",
    values = {"define": "trpc_proto_use_arena=true"},
)

# Whether to disable port reuse, it is not disabled by default.
# Note: The framework can only enable port reuse if the system itself supports it, that is,
# if "#if defined(SO_REUSEPORT)" is true.
config_setting(
    name = "trpc_disable_reuseport",
    values = {"define": "trpc_disable_reuseport=true"},
)

# Disable TCP_CORK, which is not disabled by default in the framework.
config_setting(
    name = "trpc_disable_tcp_cork",
    values = {"define": "trpc_disable_tcp_cork=true"},
)

# Enable SSL/TLS, which is turned off by default in the framework.
config_setting(
    name = "include_ssl",
    values = {"define": "include_ssl=true"},
)

config_setting(
    name = "trpc_include_ssl",
    values = {"define": "trpc_include_ssl=true"},
)

# Whether to enable asynchronous IO, it is turned off by default in the framework.
config_setting(
    name = "trpc_include_async_io",
    values = {"define": "trpc_include_async_io=true"},
)

# Enables profiler by setting this compiling option. Profiler is disabled by default in the framework.
# (When enabled, it links to the system libtcmalloc_and_profiler.so by default)
config_setting(
    name = "trpc_enable_profiler",
    values = {"define": "trpc_enable_profiler=true"},
)

# Enables profiler by setting this compiling option. Profiler is disabled by default in the framework.
# Inorder to avoid potential library version conflicts.
# It links to a user-defined version instead of linking to default system libtcmalloc_and_profiler.so.
config_setting(
    name = "trpc_enable_profiler_v2",
    values = {"define": "trpc_enable_profiler_v2=true"},
)

# Enable rpcz by setting this compiling option. Default close.
config_setting(
    name = "trpc_include_rpcz",
    values = {"define": "trpc_include_rpcz=true"},
)

# Enable prometheus by setting this compiling option. Default close.
config_setting(
    name = "trpc_include_prometheus",
    values = {"define": "trpc_include_prometheus=true"},
)

# This setting exists to maintain compatibility with old usage. Please use "trpc_include_prometheus" instead.
config_setting(
    name = "include_metrics_prometheus",
    values = {"define": "include_metrics_prometheus=true"},
)

# Enable overload control by setting this compiling option. Default close.
config_setting(
    name = "trpc_include_overload_control",
    values = {"define": "trpc_include_overload_control=true"},
)
