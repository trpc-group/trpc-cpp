licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "base64",
    hdrs = ["base64.h"],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "base64_test",
    srcs = ["base64_test.cc"],
    deps = [
        ":base64",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "body_params",
    hdrs = ["body_params.h"],
    deps = [
        "//trpc/util/http:parameter",
    ],
)

cc_library(
    name = "common",
    hdrs = ["common.h"],
    deps = [
        ":method",
        ":parameter",
        ":path",
        ":url",
        "//trpc/compressor:compressor_type",
    ],
)

cc_library(
    name = "content",
    srcs = ["content.cc"],
    hdrs = ["content.h"],
    deps = [
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "exception",
    hdrs = ["exception.h"],
    deps = [
        ":status",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "exception_test",
    srcs = ["exception_test.cc"],
    deps = [
        ":exception",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "field_map",
    hdrs = ["field_map.h"],
)

cc_test(
    name = "field_map_test",
    srcs = ["field_map_test.cc"],
    deps = [
        ":field_map",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "function_handlers",
    hdrs = ["function_handlers.h"],
    deps = [
        ":handler",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_library(
    name = "handler",
    hdrs = ["handler.h"],
    deps = [
        ":request",
        ":response",
        "//trpc/common:status",
        "//trpc/util:ref_ptr",
    ],
)

cc_library(
    name = "header",
    hdrs = ["header.h"],
    deps = [
        ":common",
        ":field_map",
        ":method",
        ":status",
        ":url",
    ],
)

cc_library(
    name = "http_header",
    hdrs = ["http_header.h"],
    deps = [
        ":header",
    ],
)

cc_test(
    name = "header_test",
    srcs = ["header_test.cc"],
    deps = [
        ":header",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_cookie",
    srcs = ["http_cookie.cc"],
    hdrs = ["http_cookie.h"],
    deps = [
        "//trpc/util:time",
        "//trpc/util/http:util",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "http_cookie_test",
    srcs = ["http_cookie_test.cc"],
    deps = [
        ":http_cookie",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_handler",
    hdrs = ["http_handler.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":exception",
        ":handler",
        ":status",
    ],
)

cc_test(
    name = "http_handler_test",
    srcs = ["http_handler_test.cc"],
    deps = [
        ":http_handler",
        "//trpc/server:server_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_handler_groups",
    hdrs = ["http_handler_groups.h"],
    deps = [
        ":http_handler",
        ":routes",
        "//trpc/util/http/stream:http_stream_handler",
    ],
)

cc_test(
    name = "http_handler_groups_test",
    srcs = ["http_handler_groups_test.cc"],
    deps = [
        ":http_handler_groups",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_parser",
    srcs = ["http_parser.cc"],
    hdrs = ["http_parser.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":response_hdrs",
        "@com_github_h2o_picohttpparser//:picohttpparser",
    ],
)

cc_test(
    name = "http_parser_test",
    srcs = [
        "http_parser_test.cc",
    ],
    deps = [
        ":http_parser",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "match_rule",
    hdrs = ["match_rule.h"],
    deps = [
        ":handler",
        ":matcher",
        ":parameter",
    ],
)

cc_test(
    name = "match_rule_test",
    srcs = ["match_rule_test.cc"],
    deps = [
        ":match_rule",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "matcher",
    srcs = ["matcher.cc"],
    hdrs = ["matcher.h"],
    deps = [
        ":parameter",
    ],
)

cc_test(
    name = "matcher_test",
    srcs = ["matcher_test.cc"],
    deps = [
        ":matcher",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "method",
    srcs = ["method.cc"],
    hdrs = ["method.h"],
)

cc_test(
    name = "method_test",
    srcs = ["method_test.cc"],
    deps = [
        ":method",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mime_types",
    srcs = ["mime_types.cc"],
    hdrs = ["mime_types.h"],
)

cc_test(
    name = "mime_types_test",
    srcs = ["mime_types_test.cc"],
    deps = [
        ":mime_types",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "parameter",
    hdrs = ["parameter.h"],
    deps = [
        ":field_map",
        "//trpc/util:string_util",
    ],
)

cc_test(
    name = "parameter_test",
    srcs = ["parameter_test.cc"],
    deps = [
        ":parameter",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "path",
    hdrs = ["path.h"],
)

cc_test(
    name = "path_test",
    srcs = ["path_test.cc"],
    deps = [
        ":path",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "request",
    srcs = ["request.cc"],
    hdrs = ["request.h"],
    deps = [
        ":common",
        ":content",
        ":http_header",
        ":parameter",
        ":util",
        "//trpc/stream/http:http_stream_hdrs",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_test(
    name = "request_test",
    srcs = ["request_test.cc"],
    deps = [
        ":request",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "response",
    srcs = ["response.cc"],
    deps = [
        ":response_hdrs",
        "//trpc/stream/http:http_stream_hdrs",
    ],
)

cc_library(
    name = "response_hdrs",
    hdrs = [
        "reply.h",
        "response.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        ":content",
        ":http_header",
        ":mime_types",
        ":request",
        ":status",
        "//trpc/stream/http:http_stream_hdrs",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

alias(
    name = "reply",
    actual = ":response",
)

cc_test(
    name = "response_test",
    srcs = ["response_test.cc"],
    deps = [
        ":response",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "routes",
    srcs = ["routes.cc"],
    hdrs = ["routes.h"],
    deps = [
        ":exception",
        ":handler",
        ":match_rule",
        ":method",
        ":parameter",
        ":path",
        ":request",
        ":response",
        ":util",
        "//trpc/server:server_context",
    ],
)

cc_test(
    name = "routes_test",
    srcs = ["routes_test.cc"],
    deps = [
        ":routes",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "status",
    srcs = ["status.cc"],
    hdrs = ["status.h"],
)

cc_test(
    name = "status_test",
    srcs = ["status_test.cc"],
    deps = [
        ":status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "url",
    srcs = ["url.cc"],
    hdrs = ["url.h"],
)

cc_test(
    name = "url_test",
    srcs = ["url_test.cc"],
    deps = [
        ":url",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
)

cc_test(
    name = "util_test",
    srcs = [
        "util_test.cc",
    ],
    deps = [
        ":util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
