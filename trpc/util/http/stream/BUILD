licenses(["notice"])

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all_files",
    srcs = glob(
        ["**/*"],
    ),
)

cc_library(
    name = "http_client_stream_response",
    hdrs = ["http_client_stream_response.h"],
    deps = [
        "//trpc/stream/http:http_client_stream",
    ],
)

cc_library(
    name = "http_start_line",
    hdrs = ["http_start_line.h"],
)

cc_library(
    name = "http_stream_handler",
    hdrs = ["http_stream_handler.h"],
    deps = [
        ":stream_handler",
        "//trpc/server:server_context",
        "//trpc/util/http:exception",
    ],
)

cc_library(
    name = "stream_function_handlers",
    hdrs = ["stream_function_handlers.h"],
    deps = [
        ":stream_handler",
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "stream_function_handlers_test",
    srcs = ["stream_function_handlers_test.cc"],
    deps = [
        ":stream_function_handlers",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_handler",
    hdrs = ["stream_handler.h"],
    deps = [
        "//trpc/stream/http:http_stream",
        "//trpc/util/http:handler",
    ],
)
