licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "admin_handler",
    srcs = ["admin_handler.cc"],
    hdrs = ["admin_handler.h"],
    deps = [
        "//trpc/util/http:handler",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "admin_handler_test",
    srcs = ["admin_handler_test.cc"],
    deps = [
        ":admin_handler",
        "//trpc/server:server_context",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "admin_service",
    srcs = ["admin_service.cc"],
    hdrs = ["admin_service.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":admin_handler",
        ":client_detach_handler",
        ":commands_handler",
        ":contention_profiler_handler",
        ":cpu_profiler_handler",
        ":heap_profiler_handler",
        ":index_handler",
        ":js_handler",
        ":log_level_handler",
        ":prometheus_handler",
        ":reload_config_handler",
        ":sample",
        ":stats_handler",
        ":sysvars_handler",
        ":version_handler",
        ":watch_handler",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/server:http_service",
        "//trpc/util:time",
    ],
)

cc_test(
    name = "admin_service_test",
    srcs = ["admin_service_test.cc"],
    data = ["test.yaml"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":admin_service",
        "//trpc/common/config:trpc_config",
        "//trpc/server:trpc_server",
        "//trpc/util/log:logging",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "base_funcs",
    srcs = ["base_funcs.cc"],
    hdrs = ["base_funcs.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "base_funcs_test",
    srcs = ["base_funcs_test.cc"],
    deps = [
        ":base_funcs",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "client_detach_handler",
    srcs = ["client_detach_handler.cc"],
    hdrs = ["client_detach_handler.h"],
    deps = [
        ":admin_handler",
        ":base_funcs",
        "//trpc/client:trpc_client",
        "//trpc/client:trpc_service_proxy",
        "//trpc/util/http:body_params",
    ],
)

cc_test(
    name = "client_detach_handler_test",
    srcs = ["client_detach_handler_test.cc"],
    deps = [
        ":client_detach_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "commands_handler",
    srcs = ["commands_handler.cc"],
    hdrs = ["commands_handler.h"],
    deps = [
        ":admin_handler",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "commands_handler_test",
    srcs = ["commands_handler_test.cc"],
    deps = [
        ":commands_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "contention_profiler_handler",
    srcs = ["contention_profiler_handler.cc"],
    hdrs = ["contention_profiler_handler.h"],
    defines = [] + select({
        "//trpc:trpc_enable_profiler_v2": ["TRPC_ENABLE_PROFILER"],
        "//trpc:trpc_enable_profiler": ["TRPC_ENABLE_PROFILER"],
        "//conditions:default": [],
    }),
    linkopts = [] + select({
        "//trpc:trpc_enable_profiler_v2": [],
        "//trpc:trpc_enable_profiler": ["/usr/lib64/libtcmalloc_and_profiler.so"],
        "//conditions:default": [],
    }),
    deps = [
        ":admin_handler",
        ":mutex",
        ":web_css_jquery",
        "//trpc/common/config:trpc_config",
        "//trpc/log:trpc_log",
        "//trpc/util:time",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "contention_profiler_handler_test",
    srcs = ["contention_profiler_handler_test.cc"],
    deps = [
        ":contention_profiler_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cpu_profiler_handler",
    srcs = ["cpu_profiler_handler.cc"],
    hdrs = ["cpu_profiler_handler.h"],
    defines = [] + select({
        "//trpc:trpc_enable_profiler_v2": ["TRPC_ENABLE_PROFILER"],
        "//trpc:trpc_enable_profiler": ["TRPC_ENABLE_PROFILER"],
        "//conditions:default": [],
    }),
    linkopts = [] + select({
        "//trpc:trpc_enable_profiler_v2": [],
        "//trpc:trpc_enable_profiler": ["/usr/lib64/libtcmalloc_and_profiler.so"],
        "//conditions:default": [],
    }),
    deps = [
        ":admin_handler",
        ":web_css_jquery",
        "//trpc/util:time",
        "//trpc/util/http:body_params",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "cpu_profiler_handler_test",
    srcs = ["cpu_profiler_handler_test.cc"],
    deps = [
        ":cpu_profiler_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "heap_profiler_handler",
    srcs = ["heap_profiler_handler.cc"],
    hdrs = ["heap_profiler_handler.h"],
    defines = [] + select({
        "//trpc:trpc_enable_profiler_v2": ["TRPC_ENABLE_PROFILER"],
        "//trpc:trpc_enable_profiler": ["TRPC_ENABLE_PROFILER"],
        "//conditions:default": [],
    }),
    linkopts = [] + select({
        "//trpc:trpc_enable_profiler_v2": [],
        "//trpc:trpc_enable_profiler": ["/usr/lib64/libtcmalloc_and_profiler.so"],
        "//conditions:default": [],
    }),
    deps = [
        ":admin_handler",
        ":web_css_jquery",
        "//trpc/util:time",
        "//trpc/util/http:body_params",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "heap_profiler_handler_test",
    srcs = ["heap_profiler_handler_test.cc"],
    deps = [
        ":heap_profiler_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "index_handler",
    srcs = ["index_handler.cc"],
    hdrs = ["index_handler.h"],
    deps = [
        ":admin_handler",
        ":web_css_jquery",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "index_handler_test",
    srcs = ["index_handler_test.cc"],
    deps = [
        ":index_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "js_handler",
    srcs = ["js_handler.cc"],
    hdrs = ["js_handler.h"],
    deps = [
        ":admin_handler",
        ":web_css_jquery",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "js_handler_test",
    srcs = ["js_handler_test.cc"],
    deps = [
        ":js_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "log_level_handler",
    srcs = ["log_level_handler.cc"],
    hdrs = ["log_level_handler.h"],
    deps = [
        ":admin_handler",
        ":web_css_jquery",
        "//trpc/util/http:body_params",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "log_level_handler_test",
    srcs = ["log_level_handler_test.cc"],
    data = ["test.yaml"],
    deps = [
        ":log_level_handler",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mutex",
    srcs = ["mutex.cc"],
    hdrs = ["mutex.h"],
    copts = [
        "-Wall",
        "-g",
        "-fPIC",
        "-pthread",
        "-lm",
    ],
    linkopts = ["-ldl"],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "mutex_test",
    srcs = ["mutex_test.cc"],
    deps = [
        ":base_funcs",
        ":mutex",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "prometheus_handler",
    srcs = ["prometheus_handler.cc"],
    hdrs = ["prometheus_handler.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":admin_handler",
        ":base_funcs",
        "//trpc/util:prometheus",
        "//trpc/common/config:trpc_config",
        "//trpc/log:trpc_log",
        "//trpc/util/http:base64",
        "//trpc/util/string:string_helper",
    ] + select({
        "//conditions:default": [],
        "//trpc:trpc_include_prometheus": [
            "@com_github_jupp0r_prometheus_cpp//pull",
            "//trpc/metrics/prometheus:prometheus_metrics",
        ],
        "//trpc:include_metrics_prometheus": [
            "@com_github_jupp0r_prometheus_cpp//pull",
            "//trpc/metrics/prometheus:prometheus_metrics",
        ],
    }),
)

cc_test(
    name = "prometheus_handler_test",
    srcs = ["prometheus_handler_test.cc"],
    deps = [
        ":prometheus_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "reload_config_handler",
    srcs = ["reload_config_handler.cc"],
    hdrs = ["reload_config_handler.h"],
    deps = [
        ":admin_handler",
        ":web_css_jquery",
        "//trpc/common/config:trpc_config",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "reload_config_handler_test",
    srcs = ["reload_config_handler_test.cc"],
    data = ["test2.yaml"],
    deps = [
        ":reload_config_handler",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "sample",
    srcs = ["sample.cc"],
    hdrs = ["sample.h"],
    deps = [
        ":base_funcs",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "sample_test",
    srcs = ["sample_test.cc"],
    deps = [
        ":sample",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stats_handler",
    srcs = ["stats_handler.cc"],
    hdrs = ["stats_handler.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":admin_handler",
        ":web_css_jquery",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/util:string_helper",
        "//trpc/util/log:logging",
        "//trpc/tvar/common:tvar_group",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_github_open_source_parsers_jsoncpp//:jsoncpp",
    ] + select({
        "//trpc:trpc_include_rpcz": [
            "//trpc/rpcz:rpcz",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "stats_handler_test",
    srcs = ["stats_handler_test.cc"],
    data = ["test2.yaml"],
    deps = [
        ":stats_handler",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "sysvars_handler",
    srcs = ["sysvars_handler.cc"],
    hdrs = ["sysvars_handler.h"],
    deps = [
        ":admin_handler",
        ":sample",
        ":web_css_jquery",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "sysvars_handler_test",
    srcs = ["sysvars_handler_test.cc"],
    deps = [
        ":sysvars_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "version_handler",
    srcs = ["version_handler.cc"],
    hdrs = ["version_handler.h"],
    deps = [
        ":admin_handler",
        "//trpc/common:trpc_version",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "version_handler_test",
    srcs = ["version_handler_test.cc"],
    deps = [
        ":version_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "watch_handler",
    srcs = ["watch_handler.cc"],
    hdrs = ["watch_handler.h"],
    deps = [
        ":admin_handler",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "watch_handler_test",
    srcs = ["watch_handler_test.cc"],
    deps = [
        ":watch_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "web_css_jquery",
    srcs = ["web_css_jquery.cc"],
    hdrs = ["web_css_jquery.h"],
    deps = [
        ":base_funcs",
        "//trpc/admin/js:flot_min_js",
        "//trpc/admin/js:jquery_min_js",
        "//trpc/admin/js:viz_min_js",
        "//trpc/admin/perl_tools:pprof",
    ],
)

cc_test(
    name = "web_css_jquery_test",
    srcs = ["web_css_jquery_test.cc"],
    deps = [
        ":web_css_jquery",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
