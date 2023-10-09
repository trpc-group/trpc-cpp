package(
    default_visibility = ["//visibility:public"],
)

config_setting(
    name = "darwin",
    values = {
        "cpu": "darwin_x86_64",
    },
)

cc_library(
    name = "libcrypto",
    srcs = glob(["lib64/libcrypto.so"]),
    hdrs = glob(["include/openssl/*.h"]),
    includes = ["include"],
    linkopts = select({
        ":darwin": [],
        "//conditions:default": [
            "-lpthread",
            "-ldl",
        ],
    }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libssl",
    srcs = glob(["lib64/libssl.so"]),
    hdrs = glob(["include/openssl/*.h"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [":libcrypto"],
)
