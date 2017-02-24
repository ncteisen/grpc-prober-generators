cc_library(
    name = "create_prober_channel",
    srcs = ["create_prober_channel.cc"],
    hdrs = [
      "create_prober_channel.h",
      "create_test_channel.h"
    ],
    deps = [":create_test_channel"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "create_test_channel",
    srcs = ["create_test_channel.cc"],
    hdrs = [
      "create_test_channel.h",
      "test_credentials_provider.h"
    ],
    deps = [":test_credentials_provider"],
)


cc_library(
    name = "test_credentials_provider",
    srcs = ["test_credentials_provider.cc"],
    hdrs = [
      "ssl_test_data.h",
      "test_credentials_provider.h"
    ],
    deps = [":ssl_test_data"],
)

cc_library(
    name = "ssl_test_data",
    srcs = [
      "client_certs.c",
      "server1_cert.c",
      "server1_key.c",
      "test_root_cert.c",
    ],
    hdrs = [
      "ssl_test_data.h",
    ],
)
