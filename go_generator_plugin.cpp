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

  void PrintPackage(Printer &printer, vars_t &vars) const
  {
    printer.Print("package main\n\n");
  }
  
  void PrintIncludes(Printer &printer, vars_t &vars) const
  {
    printer.Print("import (\n");
    printer.Indent();
    printer.Print(
        "\"flag\"\n"
        "\"log\"\n"
        "\"net\"\n"
        "\"strconv\"\n"
        "\"fmt\"\n\n"
        "\"golang.org/x/net/context\"\n"
        "\"google.golang.org/grpc\"\n\n");
    printer.Print(
        vars, "pb \"generated_pb_files/$proto_filename_without_ext$\"\n");
    printer.Outdent();
    printer.Print(")\n\n");
  }
  
  void PrintFlags(Printer &printer, vars_t &vars) const
  {
    printer.Print("var (\n");
    printer.Indent();
    printer.Print(
      "tls                = flag.Bool(\"use_tls\", false, \"Connection uses TLS if true, else plain TCP.\")\n"
      "caFile             = flag.String(\"custom_ca_file\", \"testdata/ca.pem\", \"The file containning the CA root cert file.\")\n"
      "serverHost         = flag.String(\"server_host\", \"127.0.0.1\", \"Server host to connect to.\")\n"
      "serverPort         = flag.Int(\"server_port\", 8080, \"Server port.\")\n"
      "serverHostOverride = flag.String(\"server_host_override\", \"foo.test.google.fr\", \"The server name use to verify the hostname returned by TLS handshake.\");\n");
    printer.Outdent();
    printer.Print(")\n\n");
  }

  void DoPrintMessagePopulatingFunctionDecl(
      Printer &printer, vars_t &vars) const
  {
    // no decls needed for go
  }

  void DoPrintMessagePrintingFunctionDecl(
      Printer &printer, vars_t &vars) const
  {
     // no decls needed for go
  }

  void StartMain(Printer &printer) const
  {
    printer.Print("func main() {\n");
    printer.Indent();
  }

  void EndMain(Printer &printer) const
  {
    printer.Outdent();
    printer.Print("}\n");
  }
};


int main(int argc, char *argv[]) {
  srand(time(NULL));
  GoGrpcClientGenerator generator;
  return grpc::protobuf::compiler::PluginMain(argc, argv, &generator);
}
