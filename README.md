#gRPC Prober Generation

## Overview

gRPC already has plugins that work with `protoc` to generate the library code 
for clients and servers. This directory contains plugins that take that code
generation one step further. Given a proto file, the cpp_client_generator will
produce a c++ client file that *just works*.

The generated client should compile and run "out of the box." The requests will
be programatically populated with random sentinel data. These clients are meant to server as the skeleton files for gRPC service probers. The idea is that they are easy to read and modify to incorporate service specific logic.

## How to use

To see an example of the generator working, we will use the file `protos/helloworld.proto`

The simplest way to start is to use the generator python script. This does a lot of the metawork associated with getting the generated client running.

```
python generate.py --proto protos/helloworld --language c++ go
```

This will create two new directories, `generated_probers/helloworld_cpp` and `generated_probers/helloworld_go`. Each of these directories will contain the generated clients. To invoke each client:

```
bazel run generated_probers/helloworld_cpp:generated_helloworld_client --server_port 50051
go run generated_probers/helloworld_go/helloworld.grpc.client.pb.go --server_port 50051
```

## Design

The abstract_generator.* files define an abstract base class that handles all generation logic specific to the proto file. When if comes time to do the language specific logic, the base class calls pure virtual functions that will be overridden by per-language concrete base classes.

Adding a new language should be as simple as creating a new base class and implementing the pure virtual functions from abstract_generator.h

## Future plans

* More features
  - calls with metadata
  - async gRPC
* All supported client languages
* TLS support for connecting to already running services
* Proto population from file instead of sentinel values
