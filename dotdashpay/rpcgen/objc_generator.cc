/**
   This file borrows heavily from the grpc library. The LICENSE for
   that library is included because of the high degree of similarity:

   Copyright 2015, Google Inc.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.
   * Neither the name of Google Inc. nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#include "objc_generator.h"

#include <dotdashpay/api/common/protobuf/api_common.pb.h>
#include <google/protobuf/compiler/objectivec/objectivec_helpers.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace ddprpc_generator;

using std::map;
using std::set;
using std::string;
using std::vector;

namespace ddprpc_objc_generator {
namespace {

template <class T>
string as_string(T x) {
  std::ostringstream out;
  out << x;
  return out.str();
}

string FilenameIdentifier(const string &filename) {
  string result;
  for (unsigned i = 0; i < filename.size(); i++) {
    char c = filename[i];
    if (isalnum(c)) {
      result.push_back(c);
    } else {
      static char hex[] = "0123456789abcdef";
      result.push_back('_');
      result.push_back(hex[(c >> 4) & 0xf]);
      result.push_back(hex[c & 0xf]);
    }
  }
  return result;
}

// Taken from google/protobuf/compiler/objectivec/objectivec_primitive_field.cc
const char* PrimitiveTypeName(const google::protobuf::FieldDescriptor* descriptor) {
google::protobuf::compiler::objectivec::ObjectiveCType type =
    google::protobuf::compiler::objectivec::GetObjectiveCType(descriptor);
  switch (type) {
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_INT32:
      return "int32_t";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_UINT32:
      return "uint32_t";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_INT64:
      return "int64_t";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_UINT64:
      return "uint64_t";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_FLOAT:
      return "float";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_DOUBLE:
      return "double";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_BOOLEAN:
      return "BOOL";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_STRING:
      return "NSString";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_DATA:
      return "NSData";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_ENUM:
      return "int32_t";
  case google::protobuf::compiler::objectivec::OBJECTIVECTYPE_MESSAGE:
      return NULL;
  }

return NULL;
}

}  // namespace

string GetPrologue(const google::protobuf::FileDescriptor *file,
                   const Parameters &params, const bool& is_header) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    vars["filename"] = file->name();
    vars["filename_identifier"] = FilenameIdentifier(file->name());
    vars["filename_base"] = StripProto(file->name());

    vars["major_version"] = std::to_string(file->options().GetExtension(dotdashpay::api::common::api_major_version));
    vars["minor_version"] = std::to_string(file->options().GetExtension(dotdashpay::api::common::api_minor_version));

    printer.Print(vars, "//\n");
    printer.Print(vars, "//  Automatically generated from $filename$\n");
    printer.Print(vars, "//  DO NOT EDIT THIS FILE DIRECTLY.\n");
    printer.Print(vars, "//\n");
    printer.Print(vars, "\n");
    if (is_header) {
      printer.Print(vars, "#define DDP_API_MAJOR_VERSION $major_version$\n");
      printer.Print(vars, "#define DDP_API_MINOR_VERSION $minor_version$\n");
      printer.Print(vars, "\n");
    }
  }
  return output;
}

string GetSimulatorHeader(const google::protobuf::FileDescriptor* file,
                          const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    printer.Print(vars, "#import <Foundation/Foundation.h>\n\n");
    printer.Print(vars, "@interface DDPSimulatorMappings : NSObject\n\n");
    printer.Print(vars, "+ (NSArray*) getResponsesForRequest:(NSString*)request;\n\n");
    printer.Print(vars, "@end\n");
  }

  return output;
}

std::string GetSimulatorSource(const google::protobuf::FileDescriptor* file, const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    printer.Print(vars, "#import \"DDPSimulatorMappings.h\"\n\n");

    printer.Print(vars, "@implementation DDPSimulatorMappings\n\n");

    printer.Print(vars, "+ (NSArray*) getResponsesForRequest:(NSString*)request {\n");
    printer.Indent();
    for (int i = 0; i < file->service_count(); ++i) {
      const google::protobuf::ServiceDescriptor* service = file->service(i);
      for (int j = 0; j < service->method_count(); ++j) {
        const google::protobuf::MethodDescriptor* method = service->method(j);
        vars["MethodName"] = method->name();
        printer.Print(vars, "if ([request isEqualToString:@\"$MethodName$\"]) {\n");
        printer.Indent();

        printer.Print(vars, "return @[");

        vector<string> responses = GetUpdateResponses(method);
        for (int k = 0; k < responses.size(); ++k) {
          vars["UpdateResponseName"] = responses[k];
          printer.Print(vars, "@\"$UpdateResponseName$\", ");
        }
        vars["CompletionResponseName"] = GetCompletionResponse(method);
        printer.Print(vars, "@\"$CompletionResponseName$\"");

        printer.Print(vars, "];\n");
        printer.Outdent();
        printer.Print(vars, "}\n\n");
      }
    }

    printer.Print(vars, "return nil;\n");
    printer.Outdent();
    printer.Print(vars, "}\n\n");

    printer.Print(vars, "@end\n");
  }

  return output;
}

string GetHeaderIncludes(const google::protobuf::ServiceDescriptor* service,
                         const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    vars["ServiceMessagesIncludeFile"] = service->name() + ".pbobjc.h";
    printer.Print(vars, "#import <Foundation/Foundation.h>\n\n");
    printer.Print(vars, "#import \"DDPCallback.h\"\n\n");

    // Forward decl all of the Args and responses classes.
    set<string> classes = GetUniqueResponses(service);
    for (int i = 0; i < service->method_count(); ++i) {
      classes.insert(service->method(i)->name() + "Args");
    }

    printer.Print(vars, "@class DDPErrorResponse;\n");
    for (set<string>::iterator it = classes.begin(); it != classes.end(); ++it) {
      vars["ClassName"] = ddprpc_objc_generator::GetClassPrefix() + *it;
      printer.Print(vars, "@class $ClassName$;\n");
    }
    printer.Print(vars, "\n");
  }
  return output;
}

void PrintMethodSuffix(google::protobuf::io::Printer *printer, const bool& is_declaration) {
  if (is_declaration) {
    printer->Print(";\n\n");
  } else {
    printer->Print(" {\n");
  }
}

void PrintHeaderClientMethodInterfaces(
    google::protobuf::io::Printer *printer,
    const google::protobuf::MethodDescriptor *method,
    map<string, string> *vars,
    const bool& is_declaration) {
  (*vars)["Method"] = LowercaseFirstLetter(method->name());
  (*vars)["MethodArgs"] = ddprpc_objc_generator::GetClassPrefix() + method->name() + "Args";
  (*vars)["Request"] = google::protobuf::compiler::objectivec::ClassName(method->input_type());
  (*vars)["Response"] = google::protobuf::compiler::objectivec::ClassName(method->output_type());
  (*vars)["CompletionResponseName"] = GetCompletionResponse(method);
  (*vars)["CompletionResponseClass"]
      = ddprpc_objc_generator::GetClassPrefix() + GetCompletionResponse(method);
  const vector<string> update_responses = GetUpdateResponses(method);

  printer->Print(
      *vars,
      "- (void) $Method$:($MethodArgs$*)args "
      "onError:(void(^)(DDPErrorResponse*))callbackError on$CompletionResponseName$:(void(^)($CompletionResponseClass$*))callback$CompletionResponseName$");
  PrintMethodSuffix(printer, is_declaration);

  if (update_responses.size() > 0) {
    if (!is_declaration) {
      printer->Indent();
      printer->Print(*vars, "[self $Method$:args onError:callbackError");
      for (int i = 0; i < update_responses.size(); ++i) {
        (*vars)["UpdateResponse"] = update_responses[i];
        printer->Print(*vars, " on$UpdateResponse$:nil");
      }
      printer->Print(*vars, " on$CompletionResponseName$:callback$CompletionResponseName$];\n");
      printer->Outdent();
      printer->Print(*vars, "}\n\n");
    }

    printer->Print(*vars, "- (void) $Method$:($MethodArgs$*)args onError:(void(^)(DDPErrorResponse*))callbackError");
    for (int i = 0; i < update_responses.size(); ++i) {
      (*vars)["UpdateResponseName"] = update_responses[i];
      (*vars)["UpdateResponseClass"] = ddprpc_objc_generator::GetClassPrefix() + (update_responses[i]);
      printer->Print(
          *vars, " on$UpdateResponseName$:(void(^)($UpdateResponseClass$*))callback$UpdateResponseName$");
    }
    printer->Print(
        *vars," on$CompletionResponseName$:(void(^)($CompletionResponseClass$*))callback$CompletionResponseName$");
    PrintMethodSuffix(printer, is_declaration);
  }
}

string GetHeaderService(const google::protobuf::ServiceDescriptor* service,
                        const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    vars["Service"] = "DDP" + service->name();
    printer.Print(vars, "@interface $Service$ : NSObject\n\n");

    for (int i = 0; i < service->method_count(); ++i) {
      PrintHeaderClientMethodInterfaces(&printer, service->method(i), &vars, true);
    }

    printer.Print(vars, "\n");
    printer.Print(vars, "@end");
  }
  return output;
}


void PrintServiceMethodImplementation(google::protobuf::io::Printer *printer,
                                      const google::protobuf::MethodDescriptor *method,
                                      map<string, string> *vars) {
  (*vars)["ServiceName"] = ddprpc_objc_generator::GetClassPrefix() + method->service()->name();
  (*vars)["Method"] = LowercaseFirstLetter(method->name());
  (*vars)["MethodName"] = method->name();
  (*vars)["CompletionResponse"] = GetCompletionResponse(method);
  (*vars)["MessageId"] = ddprpc_objc_generator::GetClassPrefix() + "MessageId_" + method->name() + "Args";
  const vector<string> update_responses = GetUpdateResponses(method);

  printer->Print(*vars, "[DDPSignalManager clear:@\"$MethodName$Error\"];\n");
  for (int i = 0; i < update_responses.size(); ++i) {
    (*vars)["UpdateResponse"] = update_responses[i];
    printer->Print(*vars, "[DDPSignalManager clear:@\"$UpdateResponse$\"];\n");
  }
  printer->Print(*vars, "[DDPSignalManager clear:@\"$CompletionResponse$\"];\n");
  printer->Print(
      *vars, "[[DDPBridge getInstance] sendRequest:@\"$MethodName$\" messageId:$MessageId$ withArgs:args completionBlock:^(BOOL sent) {\n");

  printer->Indent();
  printer->Print(*vars, "VLOG(2, @\"$ServiceName$::$MethodName$: %d\", sent);\n");
  printer->Print(*vars, "if (!sent && callbackError != nil) {\n");
  printer->Indent();
  printer->Print(*vars, "DDPErrorResponse* error = [[DDPErrorResponse alloc] init];\n");
  printer->Print(*vars, "error.errorCode = @\"RequestNotAcknowledged\";\n");
  printer->Print(*vars, "error.errorMessage = @\"The request was not acknowledged. Please check the connection between this machine and the DotDashPay module.\";\n");
  printer->Print(*vars, "callbackError(error);\n");
  printer->Outdent();
  printer->Print(*vars, "} else {\n");

  printer->Indent();

  printer->Print(*vars, "if (callbackError != nil) {\n");
  printer->Indent();
  printer->Print(*vars, "[DDPSignalManager on:@\"$MethodName$Error\" performCallback:callbackError];\n");
  printer->Outdent();
  printer->Print(*vars, "}\n");

  if (update_responses.size() > 0) {
    for (int i = 0; i < update_responses.size(); ++i) {
      (*vars)["UpdateResponseName"] = update_responses[i];
      (*vars)["UpdateResponseClass"] = ddprpc_objc_generator::GetClassPrefix() + (update_responses[i]);

      printer->Print(*vars, "if (callback$UpdateResponseName$ != nil) {\n");
      printer->Indent();
      printer->Print(
          *vars, "[DDPSignalManager on:@\"$UpdateResponseName$\" performCallback:callback$UpdateResponseName$];\n");
      printer->Outdent();
      printer->Print(*vars, "}\n");
    }
  }

  printer->Print(*vars, "[DDPSignalManager on:@\"$CompletionResponse$\" performCallback:callback$CompletionResponse$];\n");
  printer->Outdent();

  printer->Print(*vars, "}\n");
  printer->Outdent();

  printer->Print(*vars, "}];\n");

}

string GetServiceImplementation(const google::protobuf::ServiceDescriptor* service,
                                const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    vars["Service"] = "DDP" + service->name();
    printer.Print(vars, "@implementation $Service$\n");
    printer.Print(vars, "\n");

    for (int i = 0; i < service->method_count(); ++i) {
      PrintHeaderClientMethodInterfaces(&printer, service->method(i), &vars, false);
      printer.Indent();
      PrintServiceMethodImplementation(&printer, service->method(i), &vars);
      printer.Outdent();
      printer.Print("}\n\n");
    }

    printer.Print(vars, "@end\n");
  }
  return output;
}

string GetHeaderEpilogue(const google::protobuf::FileDescriptor *file,
                         const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
  }
  return output;
}

string GetSourceIncludes(const google::protobuf::ServiceDescriptor* service, const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    vars["HeaderFilename"] = "DDP" + service->name() + ".h";
    printer.Print(vars, "#import \"$HeaderFilename$\"\n");
    printer.Print(vars, "\n");
    printer.Print(vars, "#import \"DDPBridge.h\"\n");
    printer.Print(vars, "#import \"DotDashPayAPI.h\"\n");
    printer.Print(vars, "#import \"DDPLogging.h\"\n");
    printer.Print(vars, "#import \"DDPSerialProtocol.h\"\n");
    printer.Print(vars, "#import \"DDPSignalManager.h\"\n\n");

    printer.Print(vars, "#import \"Common.pbobjc.h\"\n");
    const set<string> classes = GetUniqueResponses(service, true);
    for (set<string>::iterator it = classes.begin(); it != classes.end(); ++it) {
      vars["ClassName"] = *it;
      printer.Print(vars, "#import \"$ClassName$.pbobjc.h\"\n");
    }
    printer.Print(vars, "\n");
  }
  return output;
}

string GetExamplesTemplate(const google::protobuf::FileDescriptor* file, const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    printer.Print(vars, "// @example-includes(all)\n");
    printer.Print(vars, "#import <DotDashPayAPI/DotDashPayAPI.h>\n");
    printer.Print(vars, "// @example-includes-end()\n\n");

    for (int i = 0; i < file->service_count(); ++i) {
      const google::protobuf::ServiceDescriptor* service = file->service(i);
      vars["PackageName"] = service->name();
      vars["PackageNameLowercase"] = LowercaseFirstLetter(service->name());

      for (int j = 0; j < service->method_count(); ++j) {
        const google::protobuf::MethodDescriptor* method = service->method(j);
        const google::protobuf::Descriptor* args = method->input_type();
        vars["MethodName"] = method->name();
        vars["MethodNameLowercase"] = LowercaseFirstLetter(method->name());
        vars["MethodArgsName"] = ddprpc_objc_generator::GetClassPrefix() + method->name() + "Args";

        printer.Print(vars, "- (void) Example$MethodName$ {\n");
        printer.Indent();

        printer.Print(vars, "// @example-args($PackageName$.$MethodName$)\n");
        printer.Print(vars, "$MethodArgsName$* args = [[$MethodArgsName$ alloc] init];\n");
        for (int k = 0; k < args->field_count(); ++k) {
          const google::protobuf::FieldDescriptor* field = args->field(k);
          vars["OriginalFieldName"] = field->name();
          vars["FieldName"] = LowercaseFirstLetter(LowerUnderscoreToUpperCamel(field->name()));
          printer.Print(vars, "args.$FieldName$ = @example-value($OriginalFieldName$);\n");
        }
        printer.Print(vars, "// @example-args-end()\n\n");

        printer.Print(vars, "[DotDashPayAPI.$PackageNameLowercase$ $MethodNameLowercase$:args\n");
        printer.Print(vars, "// @example-error($PackageName$.$MethodName$)\n");
        printer.Print(vars, "onError:^(DDPErrorResponse* error) {\n");
        printer.Indent();

        printer.Print(vars, "if ([error.errorCode isEqualToString:@\"\"]) {\n");
        printer.Indent();

        printer.Print(vars, "LOG(ERROR, @\"%@\", error.errorMessage);\n");
        printer.Outdent();
        printer.Print(vars, "}\n");

        printer.Outdent();
        printer.Print(vars, "}\n");

        printer.Print(vars, "// @example-error-end()\n");

        vector<string> responses = GetUpdateResponses(method);
        responses.push_back(GetCompletionResponse(method));

        for (int k = 0; k < responses.size(); ++k) {
          vars["ResponseName"] = responses[k];
          vars["ResponseNameClass"] = ddprpc_objc_generator::GetClassPrefix() + responses[k];
          printer.Print(vars, "// @example-response($PackageName$.$ResponseName$)\n");
          printer.Print(vars, "on$ResponseName$:^($ResponseNameClass$* response) {\n");
          printer.Indent();

          const google::protobuf::Descriptor* response = FindMessageByName(file, responses[k]);
          if (response != NULL) {
            for (int l = 0; l < response->field_count(); ++l) {  // Damnnnn. 'l' is deep.
              const google::protobuf::FieldDescriptor* field = response->field(l);
              // Don't output META examples.
              if (field->name() == "META") {
                continue;
              }

              vars["OriginalFieldName"] = field->name();
              vars["FieldName"] = LowercaseFirstLetter(LowerUnderscoreToUpperCamel(field->name()));
              const char* type_name = PrimitiveTypeName(field);
              vars["FieldType"] = type_name ? type_name : "id";

              printer.Print(vars, "$FieldType$* $FieldName$ = response.$FieldName$;  ");
              printer.Print(vars, "// $FieldName$ = @example-value($OriginalFieldName$)\n");
            }
          } else {
            fprintf(stderr, "Could not find response defined in current context: %s\n", responses[k].c_str());
          }

          printer.Outdent();
          printer.Print(vars, "}\n");
          printer.Print(vars, "// @example-response-end()\n");
        }
        printer.Print(vars, "];\n\n");

        printer.Print(vars, "// @example-request($PackageName$.$MethodName$)\n");
        printer.Print(vars, "[DotDashPayAPI.$PackageNameLowercase$ $MethodNameLowercase$:args ");
        printer.Print(vars, "onError:^(DDPErrorResponse* error) {\n");
        printer.Indent();

        printer.Print(vars, "// Handle error response\n");
        printer.Outdent();
        printer.Print(vars, "}");

        for (int k = 0; k < responses.size(); ++k) {
          vars["ResponseName"] = responses[k];
          vars["ResponseNameClass"] = ddprpc_objc_generator::GetClassPrefix() + responses[k];
          printer.Print(vars, " on$ResponseName$:^($ResponseNameClass$* response) {\n");
          printer.Indent();

          printer.Print(vars, "// Handle response\n");
          printer.Outdent();
          printer.Print(vars, "}");
        }
        printer.Print("];\n");
        printer.Print(vars, "// @example-request-end()\n\n");
        printer.Outdent();
        printer.Print(vars, "}\n\n");

      }
    }
  }
  return output;
}

}  // namespace ddprpc_objc_generator
