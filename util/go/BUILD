load("@io_bazel_rules_go//go:def.bzl", "go_binary", "go_library")
load("@org_pubref_rules_protobuf//go:rules.bzl", "GRPC_COMPILE_DEPS")

go_library(
  name = "create_prober_channel",
  srcs = ["create_prober_channel.go"],
  deps = [
    "@org_golang_google_grpc//credentials:go_default_library"
  ] + GRPC_COMPILE_DEPS,
  visibility = ["//visibility:public"],
)
