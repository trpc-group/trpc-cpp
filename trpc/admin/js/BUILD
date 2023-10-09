licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "jquery_min_js",
    srcs = ["jquery_min_js.cc"],
    hdrs = ["jquery_min_js.h"],
)

cc_library(
    name = "viz_min_js",
    srcs = ["viz_min_js.cc"],
    hdrs = ["viz_min_js.h"],
)

cc_library(
    name = "flot_min_js",
    srcs = ["flot_min_js.cc"],
    hdrs = ["flot_min_js.h"],
)

cc_test(
    name = "jquery_min_js_test",
    srcs = ["jquery_min_js_test.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":jquery_min_js",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "flot_min_js_test",
    srcs = ["flot_min_js_test.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":flot_min_js",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "viz_min_js_test",
    srcs = ["viz_min_js_test.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":viz_min_js",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
