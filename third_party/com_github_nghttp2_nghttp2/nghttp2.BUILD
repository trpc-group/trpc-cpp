licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "nghttp2",
    srcs = glob(
        [
            "lib/**/*.c",
        ],
        exclude = ["lib/**/*_test.c"],
    ),
    hdrs = glob([
        "lib/includes/**/*.h",
        "lib/**/*.h",
    ]),
    includes = ["lib/includes"],
    linkstatic = 1,
)
