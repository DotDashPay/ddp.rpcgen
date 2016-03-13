/**
   `ddprpc_nodejs_plugin` is a binary utility for automatically generating the
   necessary API interfaces that the DotDashPay APIs are built on top
   of.

   Note that this file is called `nodejs_plugin.cc` which differs from
   the binary that is generated from this file. This is to avoid
   naming collisions on systems where a binary called `nodejs_plugin` can
   be very ambiguous.

   Use the plugin by executing the following command:

   ```bash
   protoc                                          \
   --plugin=protoc-gen-ddprpc=ddprpc_nodejs_plugin   \
   --ddprpc_out=OUT_DIR services.proto
   ```

   This will generate an interface for each service defined in
   `services.proto` that an API implementation can "subclassed" to
   ensure the implementation is consistent with the services defiend
   in `services.proto`.

   Before calling the above function, to complete the entire build
   process, you should first autogenerate `services.proto` via the
   autogenerator in `ddp.api.common` (typically the binary
   `spec/generate-services-proto.js`).

   ===============================================================

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
#include "nodejs_generator.h"

#include <dotdashpay/api/common/protobuf/api_common.pb.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <string>

using std::string;

namespace dotdashpay {
namespace rpcgen {
class NodeJSGenerator : public google::protobuf::compiler::CodeGenerator {
 public:
  NodeJSGenerator() {}
  virtual ~NodeJSGenerator() {}

  virtual bool Generate(const google::protobuf::FileDescriptor *file,
                        const string &parameter,
                        google::protobuf::compiler::GeneratorContext *context,
                        string *error) const {
    ddprpc_nodejs_generator::Parameters generator_parameters;

    if (!file->options().HasExtension(dotdashpay::api::common::api_major_version)
        || !file->options().HasExtension(dotdashpay::api::common::api_minor_version)) {
      *error = "ddprpc compiler requires that api_major_version and api_major_version "
               "are set in the options";
      return false;
    }

    // Ensure the protobuf file conforms to what we're expecting.
    if (!ddprpc_generator::IsConformant(file, error)) {
      *error = "input file is non-conformant";
      return false;
    }

    // Build each of the "service implementations".
    for (int i = 0; i < file->service_count(); ++i) {
      const google::protobuf::ServiceDescriptor* service = file->service(i);
      string file_name = ddprpc_generator::LowercaseFirstLetter(service->name());

      string source_code =
          ddprpc_nodejs_generator::GetPrologue(file, generator_parameters) +
          ddprpc_nodejs_generator::GetSourceIncludes(service, generator_parameters) +
          ddprpc_nodejs_generator::GetServiceImplementation(service, generator_parameters);
      std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> source_output(
          context->Open(file_name + ".js"));
      google::protobuf::io::CodedOutputStream source_coded_out(
          source_output.get());
      source_coded_out.WriteRaw(source_code.data(), source_code.size());
    }

    return true;
  }

 private:
  // Insert the given code into the given file at the given insertion point.
  void Insert(google::protobuf::compiler::GeneratorContext *context,
              const string &filename, const string &insertion_point,
              const string &code) const {
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
        context->OpenForInsert(filename, insertion_point));
    google::protobuf::io::CodedOutputStream coded_out(output.get());
    coded_out.WriteRaw(code.data(), code.size());
  }
};
}  // namespace rpcgen
}  // namespace dotdashpay

int main(int argc, char** argv) {
  dotdashpay::rpcgen::NodeJSGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
