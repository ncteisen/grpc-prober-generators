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
    {grpc::protobuf::FieldDescriptor::TYPE_BYTES, "make([]byte, 20)"},
    {grpc::protobuf::FieldDescriptor::TYPE_UINT32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED64, "1234"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT64, "1234"},
};

static std::map<grpc::protobuf::FieldDescriptor::Type, grpc::string> repeated_sentinel_data {
    {grpc::protobuf::FieldDescriptor::TYPE_DOUBLE, "[]float64{1.234, 5.67}"},
    {grpc::protobuf::FieldDescriptor::TYPE_FLOAT, "[]float32{1.234, 5.67}"},
    {grpc::protobuf::FieldDescriptor::TYPE_INT64, "[]int64{-12,34}"},
    {grpc::protobuf::FieldDescriptor::TYPE_UINT64, "[]uint64{12,34}"},
    {grpc::protobuf::FieldDescriptor::TYPE_INT32, "[]int32{-1,2,3}"},
    {grpc::protobuf::FieldDescriptor::TYPE_FIXED64, "[]uint64{12,34}"},
    {grpc::protobuf::FieldDescriptor::TYPE_FIXED32, "[]uint32{12,34}"},
    {grpc::protobuf::FieldDescriptor::TYPE_BOOL, "[]bool{true, false}"},
    {grpc::protobuf::FieldDescriptor::TYPE_STRING, "[]string{\"Hello\", \"world\"}"},
    {grpc::protobuf::FieldDescriptor::TYPE_BYTES, "make([]byte, 20)"},
    {grpc::protobuf::FieldDescriptor::TYPE_UINT32, "[]uint32{12,34}"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED32, "[]int32{-1,2,3}"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED64, "[]int64{-1,2,3}"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT32, "[]int32{2,3}"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT64, "[]int64{2,3}"},
};

class GoGrpcClientGenerator : public AbstractGenerator {
 private:
  grpc::string GetLanguageSpecificFileExtension() const 
  { 
    return ".grpc.client.pb.go"; 
  }

  grpc::string GetCommentPrefix() const 
  {
    return "// "; 
  }

  void DoPrintPackage(Printer &printer, vars_t &vars) const
  {
    printer.Print("package main\n\n");
  }
  
  void DoPrintIncludes(Printer &printer, vars_t &vars) const
  {
    printer.Print("import (\n");
    printer.Indent();
    printer.Print(
        "\"flag\"\n"
        "\"fmt\"\n\n"
        "\"golang.org/x/net/context\"\n"
        "\"github.com/golang/glog\"\n"
        "\"google.golang.org/grpc\"\n\n");
    printer.Print(
        vars, "pb \"github.com/ncteisen/grpc-prober-generators/generated_go_pb_files/$proto_filename_without_ext$/$proto_filename_without_ext$\"\n");
    printer.Print("util \"github.com/ncteisen/grpc-prober-generators/util/go/create_prober_channel\"\n");
    printer.Outdent();
    printer.Print(")\n\n");
  }
  
  void DoPrintFlags(Printer &printer, vars_t &vars) const
  {
    printer.Print("var (\n");
    printer.Indent();
    printer.Print(
      "useTLS             = flag.Bool(\"use_tls\", false, \"Connection uses TLS if true, else plain TCP.\")\n"
      "testCA             = flag.Bool(\"use_test_ca\", false, \"Client will use custom ca file.\")\n"
      "serverHost         = flag.String(\"server_host\", \"127.0.0.1\", \"Server host to connect to.\")\n"
      "serverPort         = flag.Int(\"server_port\", 8080, \"Server port.\")\n"
      "serverHostOverride = flag.String(\"server_host_override\", \"foo.test.google.fr\", \"The server name use to verify the hostname returned by TLS handshake.\")\n");
    printer.Outdent();
    printer.Print(")\n\n");
  }

  void DoParseFlags(Printer &printer) const
  {
    printer.Print("flag.Parse()\n");
  }

  void DoStartPrint(Printer &printer) const
  {
    printer.Print("fmt.Println(\"");
  }

  void DoEndPrint(Printer &printer) const
  {
    printer.Print("\")\n");
  }

  void DoCreateChannel(Printer &printer) const
  {
    printer.Print("channel := util.CreateProberChannel(serverHost, serverPort,"
        " serverHostOverride, useTLS, testCA)\n"
        "defer channel.Close()\n\n");
  }

  void DoPrintMethodProbeStart(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "func Probe$service_name$$method_name$("
                          "stub pb.$service_name$Client) {\n");
    printer.Indent();
  }

  void DoPrintMessagePopulatingFunctionStart(
    Printer &printer, vars_t &vars) const
  {
    printer.Print(
        vars, "func Create$message_name$() (*pb.$message_name$) {\n");
    printer.Indent();
    printer.Print(vars, "message := &pb.$message_name${}\n");
  }

  void DoPrintMessagePopulatingFunctionEnd(Printer &printer) const
  {
    printer.Print("return message\n");
    printer.Outdent();
    printer.Print("}");
  }

  void DoPrintServiceProbeStart(Printer &printer, vars_t &vars) const
  {
    printer.Print(
        vars, "func Probe$service_name$(channel *grpc.ClientConn) {\n");
    printer.Indent();
  }

  void DoCreateStub(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "stub := pb.New$service_name$Client(channel)\n");
  }

  void DoPopulateField(Printer &printer, vars_t &vars, 
      grpc::protobuf::FieldDescriptor::Type type, bool repeated) const
  {
    if (repeated) {
      vars["data"] = repeated_sentinel_data[type];
    } else {
      vars["data"] = sentinel_data[type];
    }
    printer.Print(vars, "message.$camel_case_field_name$ = $data$\n");
  }

  void DoPopulateEnum(Printer &printer, vars_t &vars, bool repeated) const
  {
    printer.Print(vars, "message.$camel_case_field_name$ = pb.$enum_name$_$upper_enum_type$\n");
  }

  void DoPopulateMessage(Printer &printer, vars_t &vars, bool repeated) const
  {
    if (repeated) {
      printer.Print(vars, "message.$camel_case_field_name$ = append(message.$camel_case_field_name$, Create$message_name$())\n");
      printer.Print(vars, "message.$camel_case_field_name$ = append(message.$camel_case_field_name$, Create$message_name$())\n");
    } else {
      printer.Print(vars, "message.$camel_case_field_name$ = Create$message_name$()\n");
    }
  }

  void DoUnaryUnary(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "request := Create$request_name$()\n\n");
    printer.Print(vars, "_, err := stub.$method_name$(context.Background(), request)\n\n");
    printer.Print("if err != nil {\n");
    printer.Indent();
    printer.Print("glog.Fatalf(\"Error occurred: %v\", err)\n");
    printer.Outdent();
    printer.Print("}\n");
  }

  void DoStartMain(Printer &printer) const
  {
    printer.Print("func main() {\n");
    printer.Indent();
  }

  void DoEndFunction(Printer &printer) const
  {
    printer.Outdent();
    printer.Print("}\n");
  }

  void DoTrailer(Printer &printer) const
  {
    // nothing for go
  }
};


int main(int argc, char *argv[]) {
  srand(time(NULL));
  GoGrpcClientGenerator generator;
  return grpc::protobuf::compiler::PluginMain(argc, argv, &generator);
}
