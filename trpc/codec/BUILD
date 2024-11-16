licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "client_codec",
    hdrs = ["client_codec.h"],
    deps = [
        ":codec_helper",
        ":protocol",
        "//trpc/common:status",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_test(
    name = "client_codec_test",
    srcs = ["client_codec_test.cc"],
    deps = [
        ":client_codec",
        "//trpc/client:client_context",
        "//trpc/codec/testing:client_codec_testing",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "client_codec_factory",
    srcs = ["client_codec_factory.cc"],
    hdrs = ["client_codec_factory.h"],
    deps = [
        ":client_codec",
    ],
)

cc_test(
    name = "client_codec_factory_test",
    srcs = ["client_codec_factory_test.cc"],
    deps = [
        ":client_codec",
        ":client_codec_factory",
        "//trpc/codec/testing:client_codec_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "codec_helper",
    srcs = ["codec_helper.cc"],
    hdrs = ["codec_helper.h"],
    deps = [
        "//trpc/codec/trpc",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "codec_helper_test",
    srcs = ["codec_helper_test.cc"],
    deps = [
        ":codec_helper",
        ":codec_manager",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "codec_manager",
    srcs = ["codec_manager.cc"],
    hdrs = ["codec_manager.h"],
    deps = [
        ":client_codec_factory",
        ":server_codec_factory",
        "//trpc/codec/grpc:grpc_client_codec",
        "//trpc/codec/grpc:grpc_server_codec",
        "//trpc/codec/http:http_client_codec",
        "//trpc/codec/http:http_server_codec",
        "//trpc/codec/redis:redis_client_codec",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/codec/trpc:trpc_server_codec",
        #"//trpc/codec/trpc_http:trpc_http_server_codec",
        "//trpc/codec/mysql:mysql_client_codec"
    ],
)

cc_test(
    name = "codec_manager_test",
    srcs = ["codec_manager_test.cc"],
    deps = [
        ":codec_manager",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "non_rpc_protocol",
    hdrs = ["non_rpc_protocol.h"],
    deps = [
        ":protocol",
    ],
)

cc_library(
    name = "protocol",
    hdrs = ["protocol.h"],
    deps = [
        "//trpc/codec/trpc",
        "//trpc/util:ref_ptr",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_test(
    name = "protocol_test",
    srcs = ["protocol_test.cc"],
    deps = [
        ":protocol",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_codec",
    hdrs = ["server_codec.h"],
    deps = [
        ":codec_helper",
        ":protocol",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_test(
    name = "server_codec_test",
    srcs = ["server_codec_test.cc"],
    deps = [
        ":server_codec",
        "//trpc/codec/testing:server_codec_testing",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_codec_factory",
    srcs = ["server_codec_factory.cc"],
    hdrs = ["server_codec_factory.h"],
    deps = [
        ":server_codec",
    ],
)

cc_test(
    name = "server_codec_factory_test",
    srcs = ["server_codec_factory_test.cc"],
    deps = [
        ":server_codec",
        ":server_codec_factory",
        "//trpc/codec/testing:server_codec_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
