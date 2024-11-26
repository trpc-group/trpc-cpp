package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "jwt-cpp",
    hdrs = glob(["**/*.h"]),
    deps = [
      "@com_github_openssl_openssl//:libcrypto",
      "@com_github_openssl_openssl//:libssl",
    ],
)
