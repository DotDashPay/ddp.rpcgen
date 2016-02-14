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

#include "cpp_generator.h"

#include <dotdashpay/api/common/protobuf/api_common.pb.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <map>
#include <sstream>
#include <string>

using std::map;
using std::string;

namespace ddprpc_cpp_generator {
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
}  // namespace

string GetHeaderPrologue(const google::protobuf::FileDescriptor *file,
                         const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    vars["filename"] = file->name();
    vars["filename_identifier"] = FilenameIdentifier(file->name());
    vars["filename_base"] = ddprpc_generator::StripProto(file->name());

    vars["major_version"] = std::to_string(file->options().GetExtension(dotdashpay::api::common::api_major_version));
    vars["minor_version"] = std::to_string(file->options().GetExtension(dotdashpay::api::common::api_minor_version));
    
    printer.Print(vars, "// Generated by the ddpRPC protobuf plugin.\n");
    printer.Print(vars, "// If you make any local change, they will be lost.\n");
    printer.Print(vars, "// source: $filename$\n");
    printer.Print(vars, "#ifndef __DOTDASHPAY_$filename_identifier$__INCLUDED\n");
    printer.Print(vars, "#define __DOTDASHPAY_$filename_identifier$__INCLUDED\n");
    printer.Print(vars, "\n");
    printer.Print(vars, "#define DDP_API_MAJOR_VERSION $major_version$\n");
    printer.Print(vars, "#define DDP_API_MINOR_VERSION $minor_version$\n");
    printer.Print(vars, "\n");
    printer.Print(vars, "#include \"$filename_base$.pb.h\"\n");
    printer.Print(vars, "\n");
  }
  return output;
}

string GetHeaderIncludes(const google::protobuf::FileDescriptor *file,
                               const Parameters &params) {
  string temp =
      "#include <dotdashpay/common/function.h>\n"
      "\n\n";

  if (!file->package().empty()) {
    std::vector<string> parts =
        ddprpc_generator::tokenize(file->package(), ".");

    for (auto part = parts.begin(); part != parts.end(); part++) {
      temp.append("namespace ");
      temp.append(*part);
      temp.append(" {\n");
    }
    temp.append("\n");
  }

  return temp;
}

void PrintHeaderClientMethodInterfaces(
    google::protobuf::io::Printer *printer,
    const google::protobuf::MethodDescriptor *method,
    map<string, string> *vars) {
  (*vars)["Method"] = method->name();
  (*vars)["Request"] = ddprpc_cpp_generator::ClassName(method->input_type(), true);
  (*vars)["Response"] = ddprpc_cpp_generator::ClassName(method->output_type(), true);

  if (method->options().HasExtension(dotdashpay::api::common::has_update_response)
      && method->options().GetExtension(dotdashpay::api::common::has_update_response)) {
    printer->Print(
        *vars,
        "virtual void $Method$(const $Request$& request, "
        "::dotdashpay::common::UpdateFunction update_handler, "
        "::dotdashpay::common::CompletionFunction completion_handler) = 0;\n");
  } else {
    printer->Print(
        *vars,
        "virtual void $Method$(const $Request$& request, "
        "::dotdashpay::common::CompletionFunction completion_handler) = 0;\n");
  }
  printer->Print("}\n");
}

void PrintHeaderService(google::protobuf::io::Printer *printer,
                        const google::protobuf::ServiceDescriptor *service,
                        map<string, string> *vars) {
  (*vars)["Service"] = service->name();

  printer->Print(*vars,
                 "class $Service$ {\n"
                 " public:\n");

  printer->Indent();
  for (int i = 0; i < service->method_count(); ++i) {
    PrintHeaderClientMethodInterfaces(printer, service->method(i), vars);
  }
  printer->Outdent();
  printer->Print("};\n");

}

string GetHeaderServices(const google::protobuf::FileDescriptor *file,
                               const Parameters &params) {
  string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer printer(&output_stream, '$');
    map<string, string> vars;

    if (!params.services_namespace.empty()) {
      vars["services_namespace"] = params.services_namespace;
      printer.Print(vars, "\nnamespace $services_namespace$ {\n\n");
    }

    for (int i = 0; i < file->service_count(); ++i) {
      PrintHeaderService(&printer, file->service(i), &vars);
      printer.Print("\n");
    }

    if (!params.services_namespace.empty()) {
      printer.Print(vars, "}  // namespace $services_namespace$\n\n");
    }
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
    map<string, string> vars;

    vars["filename"] = file->name();
    vars["filename_identifier"] = FilenameIdentifier(file->name());

    if (!file->package().empty()) {
      std::vector<string> parts =
          ddprpc_generator::tokenize(file->package(), ".");

      for (auto part = parts.rbegin(); part != parts.rend(); part++) {
        vars["part"] = *part;
        printer.Print(vars, "}  // namespace $part$\n");
      }
      printer.Print(vars, "\n");
    }

    printer.Print(vars, "\n");
    printer.Print(vars, "#endif  // __DOTDASHPAY_$filename_identifier$__INCLUDED\n");
  }
  return output;
}

}  // namespace ddprpc_cpp_generator
