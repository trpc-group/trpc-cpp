licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "string_helper",
    srcs = ["string_helper.cc"],
    hdrs = ["string_helper.h"],
    deps = [
        "//trpc/util:check",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "string_helper_test",
    srcs = ["string_helper_test.cc"],
    deps = [
        ":string_helper",
        "//trpc/util/algorithm:random",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "string_util",
    srcs = ["string_util.cc"],
    hdrs = ["string_util.h"],
    deps = [
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "string_util_test",
    srcs = ["string_util_test.cc"],
    deps = [
        ":string_util",
        "@com_google_googletest//:gtest_main",
    ],
)
