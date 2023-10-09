# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "request",
    hdrs = ["request.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "reply",
    hdrs = ["reply.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "cmdgen",
    hdrs = ["cmdgen.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "reader",
    srcs = ["reader.cc"],
    hdrs = ["reader.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":reply",
        "//trpc/common/logging:trpc_logging",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "formatter",
    srcs = ["formatter.cc"],
    hdrs = ["formatter.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":request",
    ],
)

cc_library(
    name = "redis_service_proxy",
    srcs = ["redis_service_proxy.cc"],
    hdrs = ["redis_service_proxy.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":cmdgen",
        ":formatter",
        ":reply",
        "//trpc/client:service_proxy",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec/redis:redis_client_codec",
        "//trpc/common/logging:trpc_logging",
        "//trpc/serialization:serialization_type",
        "//trpc/transport/client/common:redis_client_io_handler",
    ],
)

cc_test(
    name = "formatter_test",
    srcs = ["formatter_test.cc"],
    deps = [
        ":formatter",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "reply_test",
    srcs = ["reply_test.cc"],
    deps = [
        ":reply",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "reader_test",
    srcs = ["reader_test.cc"],
    deps = [
        ":reader",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "redis_service_proxy_test",
    srcs = ["redis_service_proxy_test.cc"],
    deps = [
        ":cmdgen",
        ":redis_service_proxy",
        "//trpc/client:make_client_context",
        "//trpc/client:service_proxy_option_setter",
        "//trpc/common:trpc_plugin",
        "//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "cmdgen_test",
    srcs = ["cmdgen_test.cc"],
    deps = [
        ":cmdgen",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)