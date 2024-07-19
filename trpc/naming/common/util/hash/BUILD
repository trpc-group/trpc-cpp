# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hash_func",
    hdrs = ["hash_func.h"],
    visibility = [
        "//visibility:public",
    ],
    linkopts =["-lssl","-lcrypto"],
    deps = [
        "//trpc/naming/common/util/hash:MurmurHash3",
        "//trpc/naming/common/util/hash:City",
    ],
)

cc_library(
    name="MurmurHash3",
    srcs=["MurmurHash3.cpp"],
    hdrs=["MurmurHash3.h"],
    visibility=[
        "//visibility:public",
    ],
    deps=[],
)

cc_library(
    name="City",
    srcs=["City.cpp"],
    hdrs=["City.h"],
    visibility=[
        "//visibility:public",
    ],
    deps=[],
)