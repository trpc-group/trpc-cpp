load("//trpc:flatbuffers.bzl", "trpc_fbs_library")
load("//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "trpc_test_proto",
    srcs = ["trpc_test.proto"],
    use_trpc_plugin = True,
)

trpc_fbs_library(
    name = "trpc_test_fbs",
    srcs = ["trpc_test.fbs"],
    flatc_args = [
        "--gen-object-api",
        "--gen-compare",
        "--no-includes",
        "--gen-mutable",
        "--reflect-names",
        "--cpp-ptr-type flatbuffers::unique_ptr",
        "--trpc",
    ],
    trpc_cpp_root_path = "",
)

cc_library(
    name = "common_funcs",
    hdrs = ["common_funcs.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/runtime",
    ],
)

cc_library(
    name = "trpc_server",
    srcs = [
        "trpc_server.cc",
    ],
    hdrs = [
        "trpc_server.h",
    ],
    deps = [
        ":common_funcs",
        ":trpc_server_transport_test_inc",
        ":trpc_test_fbs",
        ":trpc_test_proto",
        "//test/end2end/common:test_signaller",
        "//trpc/common:trpc_app",
        "//trpc/runtime",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime:init_runtime",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/runtime/threadmodel/separate:separate_thread_model",
        "//trpc/util/flatbuffers:trpc_fbs",
    ],
)

cc_library(
    name = "route_server",
    srcs = [
        "trpc_route_server.cc",
    ],
    hdrs = [
        "trpc_route_server.h",
    ],
    deps = [
        ":common_funcs",
        ":trpc_test_proto",
        "//test/end2end/common:test_signaller",
        "//trpc/common:trpc_app",
        "//trpc/server/forward:forward_rpc_method_handler",
        "//trpc/server/forward:forward_rpc_service_impl",
        "//trpc/util:string_helper",
    ],
)

cc_library(
    name = "common_test",
    hdrs = ["common_test.h"],
    deps = [
        ":route_server",
        ":trpc_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "trpc_server_transport_test_inc",
    hdrs = [
        "trpc_server_transport_test_inc.h",
    ],
)

cc_test(
    name = "trpc_fiber_test",
    srcs = [
        "trpc_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/trpc/conf/trpc_client_fiber.yaml",
        "--config=test/end2end/unary/trpc/conf/trpc_server_fiber.yaml",
        "--config=test/end2end/unary/trpc/conf/trpc_route_fiber.yaml",
    ],
    data = [
        "//test/end2end/unary/trpc/conf:unit_test_resources",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_proto_use_arena": ["TRPC_PROTO_USE_ARENA"],
                  "//conditions:default": [],
              }),
    linkopts = ["-lgcov"],
    deps = [
        ":common_funcs",
        ":common_test",
        ":trpc_server_transport_test_inc",
    ],
)

cc_test(
    name = "trpc_merge_test",
    srcs = [
        "trpc_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/trpc/conf/trpc_client_merge.yaml",
        "--config=test/end2end/unary/trpc/conf/trpc_server_merge.yaml",
        "--config=test/end2end/unary/trpc/conf/trpc_route_merge.yaml",
    ],
    data = [
        "//test/end2end/unary/trpc/conf:unit_test_resources",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_proto_use_arena": ["TRPC_PROTO_USE_ARENA"],
                  "//conditions:default": [],
              }),
    linkopts = ["-lgcov"],
    deps = [
        ":common_funcs",
        ":common_test",
        ":trpc_server_transport_test_inc",
    ],
)

cc_test(
    name = "trpc_separate_test",
    srcs = [
        "trpc_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/trpc/conf/trpc_client_separate.yaml",
        "--config=test/end2end/unary/trpc/conf/trpc_server_separate.yaml",
        "--config=test/end2end/unary/trpc/conf/trpc_route_separate.yaml",
    ],
    data = [
        "//test/end2end/unary/trpc/conf:unit_test_resources",
    ],
    defines = [
        "TRPC_INCLUDE_UDS_TEST=true",
    ] + select({
        "//trpc:trpc_proto_use_arena": ["TRPC_PROTO_USE_ARENA"],
        "//conditions:default": [],
    }),
    linkopts = ["-lgcov"],
    deps = [
        ":common_funcs",
        ":common_test",
        ":trpc_server_transport_test_inc",
    ],
)
