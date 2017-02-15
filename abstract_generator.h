/*
 *
 * Copyright 2017, Google Inc.
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

/* This file represents an abstract generator. It should handle all code 
 * generation logic that is only tied to the protoc file. All languages
 * will implement concrete base classes that fill in the language specific
 * logic.
 */

#include "config.h"

using vars_t = std::map<grpc::string, grpc::string>;

class AbstractGenerator : public grpc::protobuf::compiler::CodeGenerator {
 public:

  // this function is called by the protobuf infrastructure when the protoc
  // binary is invoked with this plugin. It will call the internal generation
  // function, then write the generated output file
  bool Generate(const grpc::protobuf::FileDescriptor *file_,
                      const grpc::string &parameter,
                      grpc::protobuf::compiler::GeneratorContext *context,
                      grpc::string *error) const;
 protected:
  // Internal printing class
  class Printer {
   public:
    Printer(grpc::string *str)
        : output_stream_(str), printer_(&output_stream_, '$') {}

    void Print(const vars_t &vars,
               const char *string_template) {
      printer_.Print(vars, string_template);
    }

    void Print(const char *string) { printer_.Print(string); }
    void NewLine() { printer_.Print("\n"); }
    void Indent() { printer_.Indent(); }
    void Outdent() { printer_.Outdent(); }

   private:
    grpc::protobuf::io::StringOutputStream output_stream_;
    grpc::protobuf::io::Printer printer_;
  };

  void PrintComment(Printer &printer, vars_t &vars, grpc::string str) const;
  void PrintComment(Printer &printer, grpc::string str) const;

 private:

  // Generates all of the includes and flags and constant variables.
  grpc::string GenerateHeaders() const;

  void PrintMessagePopulatingFunctionDecl(
    const grpc::protobuf::Descriptor *message, Printer &printer) const;
  void PrintMessagePrintingFunctionDecl(
    const grpc::protobuf::Descriptor *message, Printer &printer) const;

  void PopulateInteger(
    const grpc::protobuf::FieldDescriptor *field,
    Printer &printer, vars_t &vars) const;

  void PrintPopulateField(
    const grpc::protobuf::FieldDescriptor *field,
    Printer &printer, vars_t &vars) const;

  // NOTE vars not by ref
  void DeclareAndPopulateField(
    const grpc::protobuf::Descriptor *message, 
    Printer &printer, vars_t vars) const;
  void PopulateEnum(
    const google::protobuf::EnumDescriptor *enum_, 
    Printer &printer, vars_t vars) const;

  // prints a function responsible for populating and returning
  // the particular message type.
  void PrintMessagePopulatingFunction(
    const grpc::protobuf::Descriptor *message, Printer &printer) const;

  // prints a function that prints all items in a particular message.
  void PrintMessagePrintingFunction(
    const grpc::protobuf::Descriptor *message, Printer &printer) const;

  // Generates all of the helper functions that populate all the
  // message types defined by the input proto(s)
  grpc::string GenerateMessagePopulationFunctions() const;

  // Generates the function that probes a particular method.
  void PrintMethodProbeFunction(
      const grpc::protobuf::MethodDescriptor *method,
      Printer &printer, vars_t &vars) const;

  // Generates code that calls the method probing function
  void PrintMethodProbeCall(
    const grpc::protobuf::MethodDescriptor *method,
    Printer &printer, vars_t &vars) const;

  // Generates the function that probes a particular service by calling
  // all of the methods of that service.
  void PrintServiceProbe(
      const grpc::protobuf::ServiceDescriptor *service,
      Printer &printer) const;

  void PrintServiceProbeCall(
      const grpc::protobuf::ServiceDescriptor *service,
      Printer &printer) const;

  // Generates probing functions for all of the services of a file.
  grpc::string GenerateServiceProbeFunctions() const;

  // Generates the main function of the generated prober.
  grpc::string GenerateMain() const;

  // Main generation function. Performs all logic that can be down with a
  // proto file. Delegates the language specific logic to the concrete
  // base classes.
  grpc::string GenerateProberClient() const;

  // Returns the name of the proto file, stripped of .proto
  grpc::string GetProtoName() const;

  // template function to be overridden by all concrete base classes.
  // returns the expected file extension for that language
  virtual grpc::string GetLanguageSpecificFileExtension() const = 0;

  // return language specific comment prefix string
  virtual grpc::string GetCommentPrefix() const = 0;

  virtual void PrintPackage(Printer &printer, vars_t &vars) const = 0;
  virtual void PrintIncludes(Printer &printer, vars_t &vars) const = 0;
  virtual void PrintFlags(Printer &printer, vars_t &vars) const = 0;

  virtual void DoPrintMessagePopulatingFunctionDecl(
    Printer &printer, vars_t &vars) const = 0;
  virtual void DoPrintMessagePrintingFunctionDecl(
    Printer &printer, vars_t &vars) const = 0;

  virtual void DoPrintMessagePopulatingFunctionStart(
    Printer &printer, vars_t &vars) const = 0;

  virtual void DoPrintServiceProbeStart(
    Printer &printer, vars_t &vars) const = 0;

  virtual void DoPrintMethodProbeStart(
    Printer &printer, vars_t &vars) const = 0;

  virtual void DoPopulateInteger(Printer &printer, vars_t &vars) const = 0;
  virtual void DoPopulateString(Printer &printer, vars_t &vars) const = 0;
  virtual void DoPopulateBool(Printer &printer, vars_t &vars) const = 0;
  virtual void DoPopulateFloat(Printer &printer, vars_t &vars) const = 0;
  virtual void DoPopulateMessage(Printer &printer, vars_t &vars) const = 0;
  virtual void DoPopulateEnum(Printer &printer, vars_t &vars) const = 0;
  virtual void DoCreateStub(Printer &printer, vars_t &vars) const = 0;
  virtual void DoUnaryUnary(Printer &printer, vars_t &vars) const = 0;

  virtual void DoCreateChannel(Printer &printer) const = 0;
  virtual void DoParseFlags(Printer &printer) const = 0;

  virtual void StartMain(Printer &printer) const = 0;
  virtual void EndFunction(Printer &printer) const = 0;

  // Internal object representation of the proto file.
  // Needs to be mutable so the const Generate method can set it.
  mutable const grpc::protobuf::FileDescriptor *file;
};
