licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "logging",
    hdrs = ["logging.h"],
    deps = [
        "//trpc/common:plugin",
        "//trpc/util/log",
    ],
)

cc_library(
    name = "trpc_log",
    hdrs = ["trpc_log.h"],
    deps = [
        ":printf_like",
        ":python_like",
        ":stream_like",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "python_like",
    hdrs = [
        "python_like.h",
    ],
    deps = [
        "//trpc/util/log:python_like",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_library(
    name = "printf_like",
    hdrs = [
        "printf_like.h",
    ],
    deps = [
        "//trpc/util/log:printf_like",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_library(
    name = "stream_like",
    hdrs = [
        "stream_like.h",
    ],
    deps = [
        "//trpc/util/log:stream_like",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)
