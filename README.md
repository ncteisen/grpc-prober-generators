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
python generate.py --proto protos/helloworld.proto --language c++ go
```

This will create two new directories, `generated_probers/helloworld_cpp` and `generated_probers/helloworld_go`. Each of these directories will contain the generated clients. To invoke each client:

```
bazel run generated_probers/helloworld_cpp:generated_helloworld_client --server_port 50051
bazel run generated_probers/helloworld_go:generated_helloworld_client --server_port 50051
```

If you want to actually probe a running service, you can start up a local instance of any of the example servers from the [main grpc repo example files](https://github.com/grpc/grpc/tree/master/examples).

Take a look at the generated files. They should be quite easy to follow, and are meant to serve as starting code for probers.

## Example of Creating a Prober

To demonstrate how we envision this project being used, we will generate a client, modify it, and use it to probe gRPC's continuously running test service. First generate the prober in either Go or C++ (or both):

```
python generate.py --proto protos/interop.proto --language c++ go
```

Now connect to the alway running gRPC test service by invoking the generated prober like so:

```
bazel run generated_probers/interop_cpp:generated_interop_prober -- --server_host=216.239.32.254 --server_host_override=grpc-test.sandbox.googleapis.com --server_port=443 --use_tls=true
```

This is hitting a Google service running in our production environment!

Most likely all of the tests will not run to completion, since the generating code has no knowledge of the API specific logic. However the idea is that it should be quite easy to extend the generated prober and turn it into a fully functioning prober.


## Design

The abstract_generator.* files define an abstract base class that handles all generation logic specific to the proto file. When if comes time to do the language specific logic, the base class calls pure virtual functions that will be overridden by per-language concrete base classes.

Adding a new language should be as simple as creating a new base class and implementing the pure virtual functions from abstract_generator.h

## Future plans

* More features
  - calls with metadata
  - async gRPC
* All supported client languages
* Proto population from file instead of sentinel values
