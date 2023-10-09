licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pprof",
    srcs = ["pprof.cc"],
    hdrs = ["pprof.h"],
)

cc_test(
    name = "pprof_test",
    srcs = ["pprof_test.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":pprof",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
