# Description:
#  Memcache C++ SDK

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "libmemcached",
    srcs = glob([
        "build/src/libmemcached/libmemcached.a",
    ]),
    hdrs = glob([
        "include/libmemcached-1.0/**/*.h",
        "include/libmemcached-1.0/*.h",
    ]),
    includes = [
        "include/",
    ],
)
