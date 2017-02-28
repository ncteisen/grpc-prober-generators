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

#include "abstract_generator.h"

#include <set>
#include <algorithm>

// Static helper
static bool StripSuffix(grpc::string *filename, const grpc::string &suffix) {
  if (filename->length() >= suffix.length()) {
    size_t suffix_pos = filename->length() - suffix.length();
    if (filename->compare(suffix_pos, grpc::string::npos, suffix) == 0) {
      filename->resize(filename->size() - suffix.size());
      return true;
    }
  }

  return false;
}

// Static helper. "helloworld.proto" -> "helloworld"
static grpc::string StripProto(grpc::string filename) {
  if (!StripSuffix(&filename, ".protodevel")) {
    StripSuffix(&filename, ".proto");
  }
  return filename;
}

// Static helper.
grpc::string StringReplace(grpc::string str, const grpc::string &from,
                            const grpc::string &to, bool replace_all = true) {
  size_t pos = 0;

  do {
    pos = str.find(from, pos);
    if (pos == grpc::string::npos) {
      break;
    }
    str.replace(pos, from.length(), to);
    pos += to.length();
  } while (replace_all);

  return str;
}

// Static helper. foo.bar.test --> foo::bar::test
static grpc::string DotsToColons(const grpc::string &name) {
  return StringReplace(name, ".", "::");
}

// Static helper. Returned qualified c++ call type of protobuf descriptor
static grpc::string ClassName(const grpc::protobuf::Descriptor *descriptor) {
  // Find "outer", the descriptor of the top-level message in which
  // "descriptor" is embedded.
  const grpc::protobuf::Descriptor *outer = descriptor;
  while (outer->containing_type() != NULL) outer = outer->containing_type();

  const grpc::string &outer_name = outer->full_name();
  grpc::string inner_name = descriptor->full_name().substr(outer_name.size());

  return "::" + DotsToColons(outer_name) + StringReplace(inner_name, ".", "_");
}

grpc::string AbstractGenerator::GenerateHeaders() const
{
  grpc::string output;
  {
    // scoped printer
    Printer printer(&output);

    vars_t vars;
    vars["proto_filename"] = file->name();
    vars["proto_filename_without_ext"] = StripProto(file->name());

    PrintComment(printer, "Generated by the gRPC client protobuf plugin.");
    PrintComment(printer, "If you make any local change, they will be lost.");
    PrintComment(printer, vars, "source: $proto_filename$");

    printer.NewLine();

    DoPrintPackage(printer, vars);
    DoPrintIncludes(printer, vars);
    DoPrintFlags(printer, vars);
  }
  return output;
}

static void RecursivlyTrackMessages(
  const grpc::protobuf::Descriptor* message,
  std::set<const grpc::protobuf::Descriptor*> &message_set)
{
  // already tracking this method
  if (message_set.find(message) != message_set.end())
    return;

  message_set.insert(message);

  for (int i = 0; i < message->field_count(); ++i) {
    auto field = message->field(i);
    if (field->type() == grpc::protobuf::FieldDescriptor::Type::TYPE_MESSAGE) {
      RecursivlyTrackMessages(field->message_type(), message_set);
    }
  }
}

void AbstractGenerator::PopulateEnum(
  const google::protobuf::EnumDescriptor* enum_,
  Printer &printer, vars_t vars, bool repeated) const
{
  const google::protobuf::EnumValueDescriptor* val = 
      enum_->FindValueByNumber(0); // grabs first val of enum
  vars["enum_type"] = DotsToColons(val->full_name());
  vars["enum_short_name"] = val->name();
  grpc::string enum_type_upper = val->name();
  std::transform(
      enum_type_upper.begin(), enum_type_upper.end(), enum_type_upper.begin(), toupper);
  vars["upper_enum_type"] = enum_type_upper;
  vars["enum_name"] = enum_->name();
  DoPopulateEnum(printer, vars, repeated);
}

static grpc::string to_camel_case(const grpc::string &in)
{
  grpc::string out;
  bool breaker = true;
  for (int i = 0; i < in.length(); ++i) {
    if (in[i] == '_') {
      breaker = true;
      continue;
    } else if (breaker) {
      out.push_back(toupper(in[i]));
      breaker = false;
    } else {
      out.push_back(in[i]);
    }
  }
  return out;
}

void AbstractGenerator::PrintPopulateField(
  const grpc::protobuf::FieldDescriptor *field, 
  Printer &printer, vars_t &vars) const
{
  vars["field_name"] = field->name();
  std::string upper_field_name = field->name();
  upper_field_name = to_camel_case(upper_field_name);
  vars["camel_case_field_name"] = upper_field_name;
  bool repeated = field->is_repeated();

  if (field->type() == grpc::protobuf::FieldDescriptor::Type::TYPE_MESSAGE ||
      field->type() == grpc::protobuf::FieldDescriptor::Type::TYPE_GROUP) {
    vars["message_name"] = field->message_type()->name();
    DoPopulateMessage(printer, vars, repeated);
  } else if (field->type() == grpc::protobuf::FieldDescriptor::Type::TYPE_ENUM) {
    PopulateEnum(field->enum_type(), printer, vars, repeated);
  } else {
    DoPopulateField(printer, vars, field->type(), repeated);
  }
}

void AbstractGenerator::PrintMessagePopulatingFunction(
    const grpc::protobuf::Descriptor *message, Printer &printer) const
{
  vars_t vars;
  vars["message_type"] = ClassName(message);
  vars["message_name"] = message->name();
  vars["proto_filename_without_ext"] = StripProto(file->name());

  PrintComment(printer, vars, "Helper function for populating $message_name$ message types.");

  DoPrintMessagePopulatingFunctionStart(printer, vars);

  // some languages can't have empty function bodies. *cough cough* python.
  if (!message->field_count()) DoEmptyMessage(printer, vars);

  for (int i = 0; i < message->field_count(); ++i) {
    PrintPopulateField(message->field(i), printer, vars);
  }

  DoPrintMessagePopulatingFunctionEnd(printer);
  printer.NewLine();
}

grpc::string AbstractGenerator::GenerateMessagePopulationFunctions() const
{
  // First we make sets of all of the message types needed
  // by the input proto file.
  std::set<const grpc::protobuf::Descriptor*> input_messages;
  for (int i = 0; i < file->service_count(); ++i) {
    auto service = file->service(i);
    for (int j = 0; j < service->method_count(); ++j) {
      auto method = service->method(j);
      RecursivlyTrackMessages(method->input_type(), input_messages);
    }
  }

  // Print out helper functions for populating and printing message types
  grpc::string output;
  {
    Printer printer(&output);

    PrintComment(printer, "The following functions are utility functions for populating the various message types");
    PrintComment(printer, "that are used by your service. They are to be used as examples, and then extended to");
    PrintComment(printer, "implement your API specific prober logic");
    printer.NewLine();

    for (auto it = input_messages.begin(); 
        it != input_messages.end(); ++it) {
      PrintMessagePopulatingFunction(*it, printer);
    }
    printer.NewLine();
  }
  return output;
}

void AbstractGenerator::PrintMethodProbeFunction(
    const grpc::protobuf::MethodDescriptor *method,
    Printer &printer, vars_t &vars) const
{
  vars["method_name"] = method->name();
  vars["request_type"] = ClassName(method->input_type());
  vars["response_type"] = ClassName(method->output_type());
  vars["request_name"] = method->input_type()->name();
  vars["response_name"] = method->output_type()->name();

  DoPrintMethodProbeStart(printer, vars);
  DoStartPrint(printer);
  printer.Print(vars, "\\tProbing $method_name$...");
  DoEndPrint(printer);
  printer.NewLine();

  if (method->client_streaming() || method->server_streaming()) {
    PrintComment(printer, "We do not support probing streaming methods at this time");
    PrintComment(printer, "Please fill this function in which your own streaming specific logic");
    PrintString(printer, vars, "\\t\\tStreaming not yet supported!!");
    DoEndFunction(printer);
    return;
  } 

  DoUnaryUnary(printer, vars);

  DoEndFunction(printer);
}

void AbstractGenerator::PrintMethodProbeCall(
    const grpc::protobuf::MethodDescriptor *method,
    Printer &printer, vars_t &vars) const
{
  vars["method_name"] = method->name();
  printer.Print(vars, "Probe$service_name$$method_name$(stub);\n");
}

void AbstractGenerator::PrintServiceProbe(
    const grpc::protobuf::ServiceDescriptor *service, Printer &printer) const
{
  // dump in all interesting per-service info
  vars_t vars;
  vars["service_name"] = service->name();
  vars["full_service_name"] = DotsToColons(service->full_name());
  vars["proto_filename_without_ext"] = StripProto(file->name());

  // generate the method probing functions
  for (int i = 0; i < service->method_count(); ++i) {
    PrintMethodProbeFunction(service->method(i), printer, vars);
    printer.NewLine();
  }

  DoPrintServiceProbeStart(printer, vars);

  DoStartPrint(printer);
  printer.Print(vars, "Probing $service_name$:");
  DoEndPrint(printer);
  printer.NewLine();

  DoCreateStub(printer, vars);
  printer.NewLine();


  // call the method probing functions
  for (int i = 0; i < service->method_count(); ++i) {
    PrintMethodProbeCall(service->method(i), printer, vars);
  }

  DoEndFunction(printer);
  printer.NewLine();
}

grpc::string AbstractGenerator::GenerateServiceProbeFunctions() const
{
  grpc::string output;
  {
    Printer printer(&output);

    PrintComment(printer, "The following functions are responsible for probing the unary unary methods API.");
    PrintComment(printer, "Hopefully it is easy to modify these functions to test you API specific logic.");
    printer.NewLine();

    for (int i = 0; i < file->service_count(); ++i) {
      PrintServiceProbe(file->service(i), printer);
    }
  }
  return output;
}

void AbstractGenerator::PrintServiceProbeCall(
    const grpc::protobuf::ServiceDescriptor *service, Printer &printer) const
{
  // dump in all interesting per-service info
  vars_t vars;
  vars["service_name"] = service->name();
  printer.Print(vars, "Probe$service_name$(channel);\n");
}

grpc::string AbstractGenerator::GenerateMain() const
{
  grpc::string output;
  {
    Printer printer(&output);
    DoStartMain(printer);
    vars_t vars;
    PrintString(printer, vars, "Prober started");
    printer.NewLine();
    DoParseFlags(printer);
    printer.NewLine();

    PrintComment(printer, "The channel creating code is stored in the util directory.");
    DoCreateChannel(printer);

    for (int i = 0; i < file->service_count(); ++i) {
      PrintServiceProbeCall(file->service(i), printer);
    }
    printer.NewLine();
    PrintString(printer, vars, "Prober finished");
    DoEndFunction(printer);
  }
  return output;
}

grpc::string AbstractGenerator::GenerateTrailer() const
{
  grpc::string output;
  {
    Printer printer(&output);
    DoTrailer(printer);
  }
  return output;
}

grpc::string AbstractGenerator::GenerateProberClient() const
{
  return GenerateHeaders()                    +
         GenerateMessagePopulationFunctions() +
         GenerateServiceProbeFunctions()      +
         GenerateMain()                       +
         GenerateTrailer();
}

grpc::string AbstractGenerator::GetProtoName() const
{
  return StripProto(file->name());
}

void AbstractGenerator::PrintComment(Printer &printer, 
    vars_t &vars, grpc::string str) const {
  grpc::string with_prefix = GetCommentPrefix() + str + "\n";
  printer.Print(vars, with_prefix.c_str());
}

void AbstractGenerator::PrintComment(Printer &printer, 
    grpc::string str) const {
  grpc::string with_prefix = GetCommentPrefix() + str + "\n";
  printer.Print(with_prefix.c_str());
}

void AbstractGenerator::PrintString(Printer &printer, 
    vars_t &vars, grpc::string str) const {
  vars["to_send"] = str;
  DoStartPrint(printer);
  printer.Print(vars, "$to_send$");
  DoEndPrint(printer);
}

// this function is called by the protobuf infrastructure when the protoc
// binary is invoked with this plugin. It will call the internal generation
// function, then write the generated output file
bool AbstractGenerator::Generate(const grpc::protobuf::FileDescriptor *file_,
                      const grpc::string &parameter,
                      grpc::protobuf::compiler::GeneratorContext *context,
                      grpc::string *error) const
{
  file = file_; // set member so children can access it
  grpc::string generated_file = GenerateProberClient();
  grpc::string file_name = GetProtoName() + GetLanguageSpecificFileExtension();
  std::unique_ptr<grpc::protobuf::io::ZeroCopyOutputStream> client_output(
      context->Open(file_name));
  grpc::protobuf::io::CodedOutputStream client_coded_out(client_output.get());
  client_coded_out.WriteRaw(generated_file.data(), generated_file.size());
  return true;
}


