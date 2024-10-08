from build.protobuf import proto, protocc

proto(
    name="proto",
    srcs=["./imagereader.proto"],
    deps=["lib+common_proto"],
)
protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib+common_proto_lib"],
)
