/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <memory>
#include <sstream>
#include <cstdlib>
#include <map>

#include "abstract_generator.h"

static std::map<grpc::protobuf::FieldDescriptor::Type, grpc::string> sentinel_data {
    {grpc::protobuf::FieldDescriptor::TYPE_DOUBLE, "1.234"},
    {grpc::protobuf::FieldDescriptor::TYPE_FLOAT, "1.234"},
    {grpc::protobuf::FieldDescriptor::TYPE_INT64, "1234"},
    {grpc::protobuf::FieldDescriptor::TYPE_UINT64, "1234"},
    {grpc::protobuf::FieldDescriptor::TYPE_INT32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_FIXED64, "1234"},
    {grpc::protobuf::FieldDescriptor::TYPE_FIXED32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_BOOL, "true"},
    {grpc::protobuf::FieldDescriptor::TYPE_STRING, "\"Hello world\""},
    {grpc::protobuf::FieldDescriptor::TYPE_BYTES, "\"Hello world\""},
    {grpc::protobuf::FieldDescriptor::TYPE_UINT32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED64, "1234"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT64, "1234"},
};

class CppGrpcClientGenerator : public AbstractGenerator {
 private:
  grpc::string GetLanguageSpecificFileExtension() const 
  { 
    return ".grpc.client.pb.cc"; 
  }

  grpc::string GetCommentPrefix() const 
  {
    return "// "; 
  }

  void DoPrintPackage(Printer &printer, vars_t &vars) const
  {
    // nothing to do for c++
  }

  void DoPrintIncludes(Printer &printer, vars_t &vars) const
  {
    // headers
    std::vector<grpc::string> headers = {
        "iostream",
        "memory",
        "string",
        "cstdint",
        "thread",
        "gflags/gflags.h",
        "grpc++/grpc++.h",
        "grpc/support/log.h",
        "grpc/support/useful.h"};
    
    for (auto i = headers.begin(); i != headers.end(); i++) {
      vars["header"] = *i;
      printer.Print(vars, "#include <$header$>\n");
    }

    printer.Print(vars,
            "\n#include \"$proto_filename_without_ext$.grpc.pb.h\"\n"
            "\n#include \"../../util/cpp/create_prober_channel.h\"\n\n");
  }

  void DoPrintFlags(Printer &printer, vars_t &vars) const
  {
    printer.Print(
      "// In some distros, gflags is in the namespace "
      "google, and in some others,\n"
      "// in gflags. This hack is enabling us to find both.\n"
      "namespace google {}\n"
      "namespace gflags {}\n"
      "using namespace google;\n"
      "using namespace gflags;\n\n");

    // print the flag definitions
    printer.Print("DEFINE_bool(use_tls, false, \"Connection uses TLS if true, else plain TCP.\");\n"
        "DEFINE_bool(use_test_ca, false, "
            "\"Client will use custom ca file.\");\n"
        "DEFINE_int32(server_port, 8080, \"Server port.\");\n"
        "DEFINE_string(server_host, \"localhost\", "
            "\"Server host to connect to\");\n"
        "DEFINE_string(server_host_override, \"foo.test.google.fr\",\n"
        "\t\t\"The server name use to verify the hostname returned by TLS handshake\");\n\n");
  }

  void DoCreateChannel(Printer &printer) const
  {
    printer.Print(
      "std::shared_ptr<grpc::Channel> channel = grpc::CreateProberChannel(\n"
      "\t\tFLAGS_server_host, FLAGS_server_port, FLAGS_server_host_override,\n"
      "\t\tFLAGS_use_tls, FLAGS_use_test_ca);\n\n");
  }

  void DoParseFlags(Printer &printer) const
  {
    printer.Print("ParseCommandLineFlags(&argc, &argv, true);\n");
  }

  void DoStartPrint(Printer &printer) const
  {
    printer.Print("std::cout << \"");
  }

  void DoEndPrint(Printer &printer) const
  {
    printer.Print("\" << std::endl;\n");
  }

  void DoPrintMessagePopulatingFunctionStart(
      Printer &printer, vars_t &vars) const
  {
    printer.Print(
        vars, "void Populate$message_name$($message_type$ *message) {\n");
    printer.Indent();
  }

  void DoPrintMessagePopulatingFunctionEnd(Printer &printer) const
  {
    DoEndFunction(printer);
  }


  void DoPrintMethodProbeStart(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "void Probe$service_name$$method_name$("
                        "std::shared_ptr<$full_service_name$::Stub> stub) {\n");
    printer.Indent();
  }

  void DoPrintServiceProbeStart(Printer &printer, vars_t &vars) const
  {
    printer.Print(
        vars, "void Probe$service_name$(std::shared_ptr<grpc::Channel> channel) {\n");
    printer.Indent();
  }

  void DoCreateStub(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "std::shared_ptr<$full_service_name$::Stub> stub =\n"
                        "\t\t$full_service_name$::NewStub(channel);\n");
  }

  void DoPopulateField(Printer &printer, vars_t &vars, 
      grpc::protobuf::FieldDescriptor::Type type, bool repeated) const
  {
    vars["data"] = sentinel_data[type];
    if (repeated) {
      printer.Print(vars, "message->add_$field_name$($data$);\n");
      printer.Print(vars, "message->add_$field_name$($data$);\n");
    } else {
      printer.Print(vars, "message->set_$field_name$($data$);\n");
    }
  }

  void DoPopulateEnum(Printer &printer, vars_t &vars, bool repeated) const
  {
    if (repeated) {
      printer.Print(vars, "message->add_$field_name$($enum_type$);\n");
      printer.Print(vars, "message->add_$field_name$($enum_type$);\n");
    } else {
      printer.Print(vars, "message->set_$field_name$($enum_type$);\n");
    }
  }

  void DoPopulateMessage(Printer &printer, vars_t &vars, bool repeated) const
  {
    vars["mutable_or_add"] = repeated ? "add" : "mutable";
    if (repeated) {
      printer.Print(vars, "Populate$message_name$(message->add_$field_name$());\n");
      printer.Print(vars, "Populate$message_name$(message->add_$field_name$());\n");
    } else {
      printer.Print(vars, "Populate$message_name$(message->mutable_$field_name$());\n");
    }
  }

  void DoUnaryUnary(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "$request_type$ request;\n");
    printer.Print(vars, "$response_type$ response;\n");
    printer.Print("grpc::ClientContext context;\n\n");
    printer.Print(vars, "Populate$request_name$(&request);\n\n");
    printer.Print(vars, "grpc::Status status = stub->$method_name$(&context, request, &response);\n\n");
    printer.Print("GPR_ASSERT(status.ok());\n");
  }

  void DoStartMain(Printer &printer) const
  {
    printer.Print("int main(int argc, char** argv) {\n");
    printer.Indent();
  }

  void DoEndFunction(Printer &printer) const
  {
    printer.Outdent();
    printer.Print("}\n");
  }
};


int main(int argc, char *argv[]) {
  srand(time(NULL));
  CppGrpcClientGenerator generator;
  return grpc::protobuf::compiler::PluginMain(argc, argv, &generator);
}
