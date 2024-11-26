licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "align",
    hdrs = ["align.h"],
)

cc_library(
    name = "bind_core_manager",
    srcs = ["bind_core_manager.cc"],
    hdrs = ["bind_core_manager.h"],
    deps = [
        "//trpc/util/log:logging",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_test(
    name = "bind_core_manager_test",
    srcs = ["bind_core_manager_test.cc"],
    deps = [
        ":bind_core_manager",
        "//trpc/log:trpc_log",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "check",
    srcs = ["check.cc"],
    hdrs = ["check.h"],
    deps = [
        ":likely",
        "//trpc/util/log:logging",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "check_test",
    srcs = ["check_test.cc"],
    deps = [
        ":check",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "deferred",
    hdrs = ["deferred.h"],
    deps = [
        ":function",
    ],
)

cc_library(
    name = "delayed_init",
    hdrs = ["delayed_init.h"],
)

cc_library(
    name = "doubly_linked_list",
    hdrs = ["doubly_linked_list.h"],
    deps = [
        "//trpc/util/internal:doubly_linked_list",
    ],
)

cc_test(
    name = "delayed_init_test",
    srcs = ["delayed_init_test.cc"],
    deps = [
        ":delayed_init",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "domain_util",
    srcs = ["domain_util.cc"],
    hdrs = ["domain_util.h"],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "domain_util_test",
    srcs = ["domain_util_test.cc"],
    deps = [
        ":domain_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "erased_ptr",
    hdrs = ["erased_ptr.h"],
    deps = [
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "erased_ptr_test",
    srcs = ["erased_ptr_test.cc"],
    deps = [
        ":erased_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "hash_util",
    hdrs = ["hash_util.h"],
    deps = [
        "//trpc/util/algorithm:hash",
    ],
)

cc_library(
    name = "latch",
    hdrs = ["latch.h"],
    deps = [
        "//trpc/util/thread:latch",
    ],
)

cc_library(
    name = "likely",
    hdrs = ["likely.h"],
)

cc_library(
    name = "lockfree_queue",
    hdrs = ["lockfree_queue.h"],
    deps = [
        "//trpc/util/queue:lockfree_queue",
    ],
)

cc_library(
    name = "pb2json",
    srcs = ["pb2json.cc"],
    hdrs = ["pb2json.h"],
    deps = [
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "jwt",
    srcs = ["jwt.cc"],
    hdrs = ["jwt.h"],
    deps = [
        "@com_github_thalhammer_jwt_cpp//:jwt-cpp",
    ],
)

cc_test(
    name = "pb2json_test",
    srcs = ["pb2json_test.cc"],
    deps = [
        ":pb2json",
        "//trpc/util/testing:testjson_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "random",
    hdrs = ["random.h"],
    deps = [
        "//trpc/util/algorithm:random",
    ],
)

cc_library(
    name = "net_util",
    srcs = ["net_util.cc"],
    hdrs = ["net_util.h"],
    deps = [
        "//trpc/util:string_util",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "net_util_test",
    srcs = ["net_util_test.cc"],
    deps = [
        ":net_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "ref_ptr",
    hdrs = ["ref_ptr.h"],
    deps = [
        ":likely",
    ],
)

cc_test(
    name = "ref_ptr_test",
    srcs = ["ref_ptr_test.cc"],
    deps = [
        ":ref_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "function",
    hdrs = ["function.h"],
    deps = [
        "//trpc/util:align",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "function_test",
    srcs = ["function_test.cc"],
    deps = [
        ":function",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "function_view",
    hdrs = ["function_view.h"],
)

cc_test(
    name = "function_view_test",
    srcs = ["function_view_test.cc"],
    deps = [
        ":function",
        ":function_view",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "singleton",
    hdrs = ["singleton.h"],
)

cc_test(
    name = "singleton_test",
    srcs = ["singleton_test.cc"],
    deps = [
        ":singleton",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "string_helper",
    hdrs = ["string_helper.h"],
    deps = [
        "//trpc/util/string:string_helper",
    ],
)

cc_library(
    name = "string_util",
    hdrs = ["string_util.h"],
    deps = [
        "//trpc/util/string:string_util",
    ],
)

cc_library(
    name = "time",
    srcs = ["time.cc"],
    hdrs = ["time.h"],
    deps = [
        "//trpc/util/chrono:time",
    ],
)

cc_library(
    name = "unique_id",
    hdrs = ["unique_id.h"],
    deps = [
    ],
)

cc_library(
    name = "prometheus",
    srcs = ["prometheus.cc"],
    hdrs = ["prometheus.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        "//trpc/util/log:logging",
        "//trpc/admin:base_funcs",
    ] + select({
        "//conditions:default": [],
        "//trpc:trpc_include_prometheus": [
            "@com_github_jupp0r_prometheus_cpp//pull",
        ],
        "//trpc:include_metrics_prometheus": [
            "@com_github_jupp0r_prometheus_cpp//pull",
        ],
    }),
)

cc_test(
    name = "prometheus_test",
    srcs = ["prometheus_test.cc"],
    deps = [
        ":prometheus",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
