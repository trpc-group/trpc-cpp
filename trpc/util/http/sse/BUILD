# trpc/util/http/sse/BUILD
cc_library(
    name = "http_sse",
    hdrs = [
        "sse_event.h",
        "sse_parser.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/common:status",
    ],
)

cc_library(
    name = "http_sse_parser",
    srcs = [
        "sse_parser.cc",
    ],
    hdrs = [
        "sse_parser.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/common:status",
        ":http_sse",
    ],
)