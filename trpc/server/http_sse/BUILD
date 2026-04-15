# trpc/server/http_sse/BUILD
licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "http_sse_service",
    srcs = ["http_sse_service.cc"],
    hdrs = ["http_sse_service.h"],
    deps = [
        "//trpc/codec/http:http_protocol",
        "//trpc/codec/http_sse:http_sse_proto_checker",
        "//trpc/util:deferred",
        "//trpc/util/log:logging",
        "//trpc/util:time",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "//trpc/util/http:routes",
        "//trpc/server:service",
        "//trpc/util/http:http_handler_groups",
    ],
)

