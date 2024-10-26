package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "jwt-cpp",
    hdrs = glob(["**/*.h"]),
    deps = [
      "@openssl//:ssl",
      "@openssl//:crypto",
    ],
)
