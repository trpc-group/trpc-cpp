load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "stream_proto",
    srcs = ["stream.proto"],
    enable_explicit_link_proto = True,
    use_trpc_plugin = True,
)

cc_library(
    name = "sync_stream_server",
    srcs = [
        "sync_stream_server.cc",
    ],
    hdrs = [
        "sync_stream_server.h",
    ],
    deps = [
        ":stream_proto",
        "//test/end2end/common:test_signaller",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/util:time",
    ],
)

cc_library(
    name = "async_stream_server",
    srcs = [
        "async_stream_server.cc",
    ],
    hdrs = [
        "async_stream_server.h",
    ],
    deps = [
        ":stream_proto",
        "//test/end2end/common:test_signaller",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

# cc_binary(
#     name = "async_stream_server_bin",
#     deps = [
#         ":async_stream_server",
#     ],
# )

cc_library(
    name = "stream_test",
    hdrs = ["stream_test.h"],
    deps = [
        ":async_stream_server",
        ":sync_stream_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "sync_stream_test",
    srcs = [
        "sync_stream_test.cc",
    ],
    args = [
        "--client_config=test/end2end/stream/trpc/conf/sync_test/sync_stream_client.yaml",
        "--config=test/end2end/stream/trpc/conf/sync_test/sync_stream_server.yaml",
        "--config=test/end2end/stream/trpc/conf/sync_test/async_stream_server.yaml",
    ],
    data = [
        "//test/end2end/stream/trpc/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":stream_test",
    ],
)

cc_test(
    name = "async_stream_test",
    srcs = [
        "async_stream_test.cc",
    ],
    args = [
        "--client_config=test/end2end/stream/trpc/conf/async_test/async_stream_client.yaml",
        "--config=test/end2end/stream/trpc/conf/async_test/sync_stream_server.yaml",
        "--config=test/end2end/stream/trpc/conf/async_test/async_stream_server.yaml",
    ],
    data = [
        "//test/end2end/stream/trpc/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":stream_test",
    ],
)
