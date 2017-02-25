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
    {grpc::protobuf::FieldDescriptor::TYPE_BOOL, "True"},
    {grpc::protobuf::FieldDescriptor::TYPE_STRING, "\"Hello world\""},
    {grpc::protobuf::FieldDescriptor::TYPE_BYTES, "\"Hello world\""},
    {grpc::protobuf::FieldDescriptor::TYPE_UINT32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SFIXED64, "1234"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT32, "123"},
    {grpc::protobuf::FieldDescriptor::TYPE_SINT64, "1234"},
};

class PythonGrpcClientGenerator : public AbstractGenerator {
 private:
  grpc::string GetLanguageSpecificFileExtension() const 
  { 
    return ".grpc.client.pb.py"; 
  }

  grpc::string GetCommentPrefix() const 
  {
    return "# "; 
  }

  void DoPrintPackage(Printer &printer, vars_t &vars) const
  {
    // nothing to do for Python
  }

  void DoPrintIncludes(Printer &printer, vars_t &vars) const
  {
    printer.Print("from __future__ import print_function\n\n");

    printer.Print("import grpc\n\n");

    printer.Print(vars,"import $proto_filename_without_ext$_pb2\n"
                       "import $proto_filename_without_ext$_pb2_grpc\n\n");
  }

  void DoPrintFlags(Printer &printer, vars_t &vars) const
  {
    // TODO
  }

  void DoCreateChannel(Printer &printer) const
  {
    printer.Print(
      "channel = grpc.insecure_channel('localhost:50051')\n");
  }

  void DoParseFlags(Printer &printer) const
  {
    // printer.Print("ParseCommandLineFlags(&argc, &argv, true);\n");
  }

  void DoStartPrint(Printer &printer) const
  {
    printer.Print("print(\"");
  }

  void DoEndPrint(Printer &printer) const
  {
    printer.Print("\")\n");
  }

  void DoPrintMessagePopulatingFunctionStart(
      Printer &printer, vars_t &vars) const
  {
    printer.Print(
        vars, "def Populate$message_name$(message):\n");
    printer.Indent();
  }

  void DoPrintMessagePopulatingFunctionEnd(Printer &printer) const
  {
    DoEndFunction(printer);
  }


  void DoPrintMethodProbeStart(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "def Probe$service_name$$method_name$(stub):\n");
    printer.Indent();
  }

  void DoPrintServiceProbeStart(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "def Probe$service_name$(channel):\n");
    printer.Indent();
  }

  void DoCreateStub(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "stub = $proto_filename_without_ext$_pb2_grpc.$service_name$Stub(channel)\n");
  }

  void DoPopulateField(Printer &printer, vars_t &vars, 
      grpc::protobuf::FieldDescriptor::Type type, bool repeated) const
  {
    vars["data"] = sentinel_data[type];
    if (repeated) {
      printer.Print(vars, "message.$field_name$ = $data$ + message.$field_name$\n");
      printer.Print(vars, "message.$field_name$ = $data$ + message.$field_name$\n");
    } else {
      printer.Print(vars, "message.$field_name$ = $data$\n");
    }
  }

  void DoPopulateEnum(Printer &printer, vars_t &vars, bool repeated) const
  {
    // if (repeated) {
    //   printer.Print(vars, "message->add_$field_name$($enum_type$);\n");
    //   printer.Print(vars, "message->add_$field_name$($enum_type$);\n");
    // } else {
    //   printer.Print(vars, "message->set_$field_name$($enum_type$);\n");
    // }
  }

  void DoPopulateMessage(Printer &printer, vars_t &vars, bool repeated) const
  {
    // vars["mutable_or_add"] = repeated ? "add" : "mutable";
    // if (repeated) {
    //   printer.Print(vars, "Populate$message_name$(message->add_$field_name$());\n");
    //   printer.Print(vars, "Populate$message_name$(message->add_$field_name$());\n");
    // } else {
    //   printer.Print(vars, "Populate$message_name$(message->mutable_$field_name$());\n");
    // }
  }

  void DoUnaryUnary(Printer &printer, vars_t &vars) const
  {
    printer.Print(vars, "request = $proto_filename_without_ext$_pb2.$request_name$()\n");
    printer.Print(vars, "Populate$request_name$(request)\n\n");
    printer.Print(vars, "response = stub.$method_name$(request);\n\n");
  }

  void DoStartMain(Printer &printer) const
  {
    printer.Print("def main():\n");
    printer.Indent();
  }

  void DoEndFunction(Printer &printer) const
  {
    printer.Outdent();
    printer.NewLine();
  }

  void DoTrailer(Printer &printer) const
  {
    printer.Print("if __name__ == '__main__':\n");
    printer.Indent();
    printer.Print("main()\n");
    printer.Outdent();
  }
};


int main(int argc, char *argv[]) {
  srand(time(NULL));
  PythonGrpcClientGenerator generator;
  return grpc::protobuf::compiler::PluginMain(argc, argv, &generator);
}
