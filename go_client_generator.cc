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

#include <map>

#include "go_client_generator.h"

#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace grpc_cpp_client_generator {

// anonymous namespace
namespace {

// These two forward declaration are needed because of the recursive nature of
// messages. The print and populate field functions need to know how to print
// and populate messages in case one of the fields is a message
void PrintMessage(Printer* printer, const Message* message, 
    std::map<grpc::string, grpc::string> *vars);
void PopulateMessage(Printer* printer, const Message* message, 
    std::map<grpc::string, grpc::string> *vars, bool dot_dereference = false);

template <class T, size_t N>
T *array_end(T (&array)[N]) {
  return array + N;
}

void AddMessage(Printer* printer, const Message* message, std::map<grpc::string,
    grpc::string> vars, bool repeated = false) {
  vars["add_or_mut"] = repeated ? "add_" : "mutable_";
  printer->Print(vars, "$field_type$* $field_name$ = "
      "$parent_input_message_name$$deref$$add_or_mut$$base_field_name$();\n\n");
  printer->Print(vars, "// populating the message $field_name$\n");
  vars["parent_input_message_name"] = vars["field_name"];
  PopulateMessage(printer, message, &vars);
}

void PopulateMessageField(Printer* printer, const Field* field, 
    std::map<grpc::string, grpc::string> *vars) {
  std::unique_ptr<const Message> message = field->message_type();
  (*vars)["field_type"] = message->type_name();
  if (field->is_repeated()) {
    (*vars)["field_name"] = field->name() + "1";
    AddMessage(printer, message.get(), *vars, true);
    (*vars)["field_name"] = field->name() + "2";
    printer->Print("\n");
    AddMessage(printer, message.get(), *vars, true);
  } else {
    AddMessage(printer, message.get(), *vars);
  }
  printer->Print("\n");
}

void DeclareEnumField(Printer* printer, const Field* field, 
    std::map<grpc::string, grpc::string> *vars) {

  std::unique_ptr<const Enum> proto_enum = field->enum_type();
  (*vars)["enum_type"] = proto_enum->type_name();
  (*vars)["value_type"] = proto_enum->random_value_type();

  printer->Print(*vars, "$enum_type$ $field_name$ = $value_type$;\n");

}

std::string GetRandomSentinelInteger() {
  std::vector<std::string> sentinels {
    "12345",
    "80808",
    "10000"
  };
  return sentinels[rand() % sentinels.size()];
}

std::string GetRandomSentinelDouble() {
  std::vector<std::string> sentinels {
    "3.1415",
    "1.6190",
    "123.321"
  };
  return sentinels[rand() % sentinels.size()];
}

std::string GetRandomSentinelString() {
  std::vector<std::string> adjectives {
    "hilarious ",
    "stealthy ",
    "finite ",
    "ingratiating "
  };
  std::vector<std::string> nouns {
    "tiger",
    "lamp",
    "turnip",
    "company"
  };

  return adjectives[rand() % adjectives.size()] + 
    nouns[rand() % nouns.size()];;
}

void PopulateField(Printer* printer, const Field* field, 
  std::map<grpc::string, grpc::string> *vars, bool dot_dereference) {

  (*vars)["field_name"] = field->name();
  (*vars)["base_field_name"] = field->name();
  (*vars)["deref"] = dot_dereference ? "." : "->";
  (*vars)["maybe_repeated"] = field->is_repeated() ? "repeated " : "";

  printer->Print(*vars, "// adding the $maybe_repeated$field $field_name$\n");

  switch (field->type()) {

    case Field::INT32:
    case Field::INT64:
    case Field::UINT32:
    case Field::UINT64:
      (*vars)["random_integer"] = GetRandomSentinelInteger();
      printer->Print(*vars, "int $field_name$ = $random_integer$;\n");
      break;

    case Field::DOUBLE:
    case Field::FLOAT:
      (*vars)["random_double"] = GetRandomSentinelDouble();
      printer->Print(*vars, "double $field_name$ = $random_double$;\n");
      break;

    case Field::BOOL:
      (*vars)["tf"] = rand() % 1 ? "true" : "false";
      printer->Print(*vars, "bool $field_name$ = $tf$;\n");
      break;

    case Field::ENUM:
      DeclareEnumField(printer, field, vars);
      break;

    case Field::STRING:
      (*vars)["randome_string"] = GetRandomSentinelString();
      printer->Print(*vars, "std::string $field_name$ = \"$randome_string$\";\n");
      break;

    case Field::MESSAGE:
      PopulateMessageField(printer, field, vars);
      break;

    default:
      printer->Print(*vars, "default;\n");
      break;
    }

    if (field->type() != Field::MESSAGE && field->type()) {
      if (field->is_repeated()) {
        printer->Print(*vars, "$parent_input_message_name$$deref$"
              "add_$field_name$($field_name$);\n");
        printer->Print(*vars, "$parent_input_message_name$$deref$"
              "add_$field_name$($field_name$);\n\n");
      } else {
        printer->Print(*vars, "$parent_input_message_name$$deref$"
              "set_$field_name$($field_name$);\n\n");
      }
    }
}

void PopulateMessage(Printer* printer, const Message* message, 
  std::map<grpc::string, grpc::string> *vars, bool dot_dereference) {

  (*vars)["message_name"] = message->name();

  printer->Print(*vars, "$parent_input_message_name$ := &pb.$message_name${}\n");

  // for (int i = 0; i < message->field_count(); ++i) {
  //   PopulateField(printer, message->field(i).get(), vars, dot_dereference);
  // }
}


void PopulateMessageWithComments(Printer* printer, const Message* message, 
  std::map<grpc::string, grpc::string> *vars) {

  (*vars)["tabs"] = "";
  printer->Print(
      "// Here we recursively populate the request message with random data.\n"
      "// This would be a good section to modify with data that makes\n"
      "// more sense for your service specifically.\n");
  PopulateMessage(printer, message, vars, true);
  printer->Print("// Done populating the request type\n\n");

}

void PrintMessageField(Printer* printer, const Message* message, 
    std::map<grpc::string, grpc::string> vars, int index = -1) {

  vars["maybe_index"] = index == -1 ? "" : "[" + std::to_string(index) + "]";
  printer->Print(vars, "$field_type$ $field_name$ = "
      "$parent_output_message_name$.$base_field_name$()$maybe_index$;\n\n");
  printer->Print(vars, "// print the message $field_name$\n");
  vars["parent_output_message_name"] = vars["field_name"];
  PrintMessage(printer, message, &vars);
}

void PrintField(Printer* printer, const Field* field, 
  std::map<grpc::string, grpc::string> *vars) {

  std::string field_name = field->name();
  field_name[0] = toupper(field_name[0]);
  (*vars)["field_name"] = field_name;
  (*vars)["base_field_name"] = field->name();
  
  if (field->type() == Field::MESSAGE) {

    // scope the message
    printer->Print("{\n");
    printer->Indent();

    std::unique_ptr<const Message> message = field->message_type();
    (*vars)["field_type"] = message->type_name();
    if (field->is_repeated()) {
      (*vars)["field_name"] = field->name() + "1";
      PrintMessageField(printer, message.get(), *vars, 0);
      (*vars)["field_name"] = field->name() + "2";
      printer->Print("\n");
      PrintMessageField(printer, message.get(), *vars, 1);
    } else {
      PrintMessageField(printer, message.get(), *vars);
    }

    printer->Outdent();
    printer->Print("}\n\n");
    
  } else {
    if (field->is_repeated()) {
      printer->Print(*vars, "std::cout << "
          "\"$tabs$$parent_output_message_name$.$field_name$()[0] = \" "
          "<< $parent_output_message_name$.$field_name$()[0] << \"\\n\";\n");
      printer->Print(*vars, "std::cout << "
          "\"$tabs$$parent_output_message_name$.$field_name$()[1] = \" "
          "<< $parent_output_message_name$.$field_name$()[1] << \"\\n\";\n");
    } else {
      printer->Print(*vars, "fmt.Printf(\"$tabs$$parent_output_message_name$.$field_name$ = %v\\n\", $parent_output_message_name$.$field_name$)\n");
    }
  }
}

void PrintMessage(Printer* printer, const Message* message, 
  std::map<grpc::string, grpc::string> *vars) {
  (*vars)["tabs"] = (*vars)["tabs"] + "\\t";
  for (int i = 0; i < message->field_count(); ++i) {
    PrintField(printer, message->field(i).get(), vars);
  }
  (*vars)["tabs"].pop_back();
  (*vars)["tabs"].pop_back();

}

void PrintMessageWithComments(Printer* printer, const Message* message, 
  std::map<grpc::string, grpc::string> *vars) {

  printer->Print(
      "// Recursively print all elements of the response message type\n");
  PrintMessage(printer, message, vars);
  printer->Print("// Done printing response\n\n");

}

void PrintErrorStatus(Printer *printer, 
    std::map<grpc::string, grpc::string> *vars) {
  printer->Indent();
  printer->Print(*vars, "std::cout << \"\\tAn error was encountered "
                        "while performing the RPC $Method$\" << std::endl;\n");
  printer->Print("std::cout << \"\\tError code: \" << status.error_code() "
          "<< \", Error message: \" << status.error_message() << std::endl;\n");
  printer->Outdent();
}

void PrintClientMethod(
    Printer *printer, const Method *method,
    std::map<grpc::string, grpc::string> *vars) {

  std::unique_ptr<const Message> input_message = method->input_message();
  std::unique_ptr<const Message> output_message = method->output_message();

  (*vars)["Method"] = method->name();
  (*vars)["Request"] = input_message->type_name();
  (*vars)["Response"] = output_message->type_name();

  std::string input_name = input_message->name();
  std::transform(
      input_name.begin(), input_name.end(), input_name.begin(), tolower);
  (*vars)["parent_input_message_name"] = input_name + "_request";

  std::string output_name = output_message->name();
  std::transform(
      output_name.begin(), output_name.end(), output_name.begin(), tolower);
  (*vars)["parent_output_message_name"] = output_name + "_response";

  printer->Print(*vars, "func $Service$$Method$($Service_lowercase$_client pb.$Service$Client) {\n\n");
  printer->Indent();

  if (method->NoStreaming()) {

    PopulateMessageWithComments(printer, method->input_message().get(), vars);

    printer->Print("// This is where the actual RPC is performed\n");
    printer->Print(*vars, "$parent_output_message_name$, err := $Service_lowercase$_client.$Method$(context.Background(), $parent_input_message_name$)\n\n");
    printer->Print("if err != nil {\n");
    printer->Indent();
    printer->Print("log.Fatalf(\"Error occurred: %v\", err)\n");
    printer->Outdent();
    printer->Print("}\n\n");

    
    // recursively print the response
    PrintMessageWithComments(printer, method->output_message().get(), vars);

  } else if (method->ClientOnlyStreaming()) {

    printer->Print(
        *vars, "std::unique_ptr<ClientWriter<$Request$> > writer(\n");
    printer->Indent();
    printer->Print(*vars,
        "stub_->$Method$(&context, &$parent_output_message_name$));\n\n");

    printer->Print("// Send multiple requests to the server\n");
    printer->Print("for (int i = 0; i < 5; ++i) {\n\n");
    printer->Indent();

    PopulateMessageWithComments(printer, method->input_message().get(), vars);

    printer->Print(
        *vars, "if (!writer->Write($parent_input_message_name$)) {\n");
    printer->Indent();
    printer->Print("std::cout << \"\\tBroken stream\" << std::endl;\n");
    printer->Outdent();
    printer->Print("}\n\n");

    printer->Outdent();
    printer->Print("}\n\n");

    printer->Print("writer->WritesDone();\n");
    printer->Print("Status status = writer->Finish();\n\n");

    printer->Print("if (status.ok()) {\n\n");
    printer->Indent();
    
    // recursively print the response
    PrintMessageWithComments(printer, method->output_message().get(), vars);

    printer->Outdent();
    printer->Print("} else {\n");
    PrintErrorStatus(printer, vars);
    printer->Print("}\n");    

  } else if (method->ServerOnlyStreaming()) {

    // recursively populate the Request
    PopulateMessageWithComments(printer, input_message.get(), vars);

    printer->Print("// This is where the actual RPC is performed\n");
    printer->Print(
        *vars, "std::unique_ptr<ClientReader<$Response$>> reader(\n");
    printer->Indent();
    printer->Print(
        *vars, "stub_->$Method$(&context, $parent_input_message_name$));\n\n");
    printer->Outdent();

    printer->Print("// Loop through all responses from the server.\n");
    printer->Print(
        *vars, "while (reader->Read(&$parent_output_message_name$)) {\n\n");
    printer->Indent();

    PrintMessageWithComments(printer, method->output_message().get(), vars);

    printer->Outdent();
    printer->Print("}\n\n");

    printer->Print("Status status = reader->Finish();\n");
    printer->Print("if (status.ok()) {\n\n");
    printer->Indent();
    printer->Print(
        *vars, "std::cout << \"\\t$Method$ rpc succeeded\" << std::endl;\n");
    printer->Outdent();
    printer->Print("} else {\n");
    PrintErrorStatus(printer, vars);
    printer->Print("}\n");


  } else if (method->BidiStreaming()) {

    printer->Print("// create the bidirectional stream\n");
    printer->Print(*vars, 
        "std::shared_ptr<ClientReaderWriter<$Request$, $Response$>> stream(\n");
    printer->Indent();
    printer->Print(*vars, "stub_->$Method$(&context));\n\n");
    printer->Outdent();

    printer->Print("// start a separate thread for writing data. "
                    "This current thread will receive data\n");
    printer->Print(*vars,
        "std::thread writer([stream, &$parent_input_message_name$]() {\n\n");
    printer->Indent();
    printer->Print("for (int i = 0; i < 5; ++i) {\n\n");
    printer->Indent();
    PopulateMessageWithComments(printer, method->input_message().get(), vars);
    printer->Print(*vars, "stream->Write($parent_input_message_name$);\n");
    printer->Outdent();
    printer->Print("}\n");
    printer->Print("stream->WritesDone();\n");
    printer->Outdent();
    printer->Print("});\n\n");

    printer->Print(
        *vars, "while (stream->Read(&$parent_output_message_name$)) {\n");
    printer->Indent();
    PrintMessageWithComments(printer, method->output_message().get(), vars);
    printer->Outdent();
    printer->Print("}\n\n");

    printer->Print("writer.join();\n");
    printer->Print("Status status = stream->Finish();\n\n");

    printer->Print("if (!status.ok()) {\n");
    PrintErrorStatus(printer, vars);
    printer->Print("}\n");
  }

  printer->Outdent();
  printer->Print("}");
}

void PrintClientServiceImpl(Printer *printer, const Service *service,
                        std::map<grpc::string, grpc::string> *vars) {

  std::string service_name = service->name();
  (*vars)["Service"] = service_name;
  std::transform(
      service_name.begin(), service_name.end(), service_name.begin(), tolower);
  (*vars)["Service_lowercase"] = service_name;

  for (int i = 0; i < service->method_count(); ++i) {
    printer->Print("\n\n");
    PrintClientMethod(printer, service->method(i).get(), vars);
  }
}

void PrintClientService(Printer *printer, const Service *service,
                        std::map<grpc::string, grpc::string> *vars) {

  std::string service_name = service->name();
  (*vars)["Service"] = service_name;
  std::transform(
      service_name.begin(), service_name.end(), service_name.begin(), tolower);
  (*vars)["Service_lowercase"] = service_name;

  printer->Print(*vars, 
      "$Service_lowercase$_client := pb.New$Service$Client(conn)\n\n");

  for (int i = 0; i < service->method_count(); ++i) {
    (*vars)["method_name"] = service->method(i)->name();
    printer->Print(*vars, "fmt.Printf(\"Calling $Service$.$method_name$:\")\n");
    printer->Print(*vars, "$Service$$method_name$($Service_lowercase$_client);\n");
    printer->Print(*vars, "fmt.Printf(\"Done calling $Service$.$method_name$:\")\n");
  }

}

void PrintChannelCreatorFunction(Printer* printer) {
  printer->Print("std::shared_ptr<Channel> CreateChannel() {\n");
  printer->Indent();
  printer->Print("const int host_port_buf_size = 1024;\n"
                 "char host_port[host_port_buf_size];\n"
                 "snprintf(host_port, host_port_buf_size, \"%s:%d\", "
                    "FLAGS_server_host.c_str(), FLAGS_server_port);\n"
                 "return grpc::CreateChannel(host_port, "
                    "grpc::InsecureChannelCredentials());\n");
  printer->Outdent();
  printer->Print("}\n\n");
}

} // anon namespace

grpc::string GetClientHeaders(File *file, const Parameters &params) {
  grpc::string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    auto printer = file->CreatePrinter(&output);
    std::map<grpc::string, grpc::string> vars;

    vars["filename_base"] = file->filename_without_ext();
    vars["service_header_ext"] = file->service_header_ext();
    vars["Package"] = file->package_with_colons();

    vars["filename"] = file->filename();

    printer->Print(vars, "// Generated by the gRPC client protobuf plugin.\n");
    printer->Print(vars,
                   "// If you make any local change, they will be lost.\n");
    printer->Print(vars, "// source: $filename$\n");

    // imports
    printer->Print("package main\n\n");

    printer->Print("import (\n");
    printer->Indent();
    printer->Print(
        "\"flag\"\n"
        "\"log\"\n"
        "\"fmt\"\n\n"
        "\"golang.org/x/net/context\"\n"
        "\"google.golang.org/grpc\"\n\n");
    printer->Print(vars, "pb \"generated_pb_files/$filename_base$\"\n");
    printer->Outdent();
    printer->Print(")\n\n");

    printer->Print("var (\n");
    printer->Indent();
    printer->Print(
      "tls                = flag.Bool(\"use_tls\", false, \"Connection uses TLS if true, else plain TCP.\")\n"
      "caFile             = flag.String(\"custom_ca_file\", \"testdata/ca.pem\", \"The file containning the CA root cert file.\")\n"
      "serverAddr         = flag.String(\"server_host\", \"127.0.0.1\", \"Server host to connect to.\")\n"
      "serverPort         = flag.String(\"server_port\", \"8080\", \"Server port.\")\n"
      "serverHostOverride = flag.String(\"server_host_override\", \"foo.test.google.fr\", \"The server name use to verify the hostname returned by TLS handshake.\");\n");
    printer->Outdent();
    printer->Print(")\n\n");

  }
  return output;
}

// Prints the client implementations classes and main function
grpc::string GetClientBody(File *file, const Parameters &params) {
  grpc::string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    auto printer = file->CreatePrinter(&output);
    std::map<grpc::string, grpc::string> vars;

    // prints all of the service's implementation classes
    for (int i = 0; i < file->service_count(); ++i) {
      PrintClientServiceImpl(printer.get(), file->service(i).get(), &vars);
      printer->Print("\n");
    }

    // print main
    printer->Print("func main() {\n\n");
    printer->Indent();

    printer->Print("flag.Parse()\n\n");

    printer->Print("conn, err := grpc.Dial(\"localhost:50051\", grpc.WithInsecure())\n"
        "if err != nil {\n"
        "\tlog.Fatalf(\"did not connect: %v\", err)\n"
        "}\n"
        "defer conn.Close()\n\n");


    // calls all of the methods of every service
    for (int i = 0; i < file->service_count(); ++i) {
      PrintClientService(printer.get(), file->service(i).get(), &vars);
      printer->Print("\n");
    }

    printer->Outdent();
    printer->Print("}\n");


  }
  return output;
}


}  // namespace grpc_cpp_client_generator