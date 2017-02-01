#gRPC Client Generation

## Overview

gRPC already has plugins that work with `protoc` to generate the library code 
for clients and servers. This directory contains plugins that take that code
generation one step further. Given a proto file, the cpp_client_generator will
produce a c++ client file that *just works*.

The generated client should compile and run "out of the box." The requests will
be programatically populated with random sentinel data. This could be used for
fuzz testing, but should also serve as a good entry point to creating more
focused tests. Use the generated code as a skeleton, then change the data to
make the test useful.

## How to use

At this point the generator code only makes clients. To see an example of the
generator working, we will use the cpp helloworld example that can be found
under examples/cpp/helloworld. Starting from the top of the gRPC repo:

```
# build the plugins
make

# generate the client
cd helloworld
make

# run the generated client. Note that you may want to start up an the
# greeter server binary found in grpc/example/cpp/helloworld
./generated_client --server_host localhost --server_port 50051
```

Examine the contents of `helloworld.grpc.client.pb.cc`; it should be easy to see
whats going on.

## Future plans

* More features
  - calls with metadata
  - async gRPC

* All supported client languages
* TLS support for connecting to already running services
