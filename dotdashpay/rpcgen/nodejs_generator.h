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

#ifndef __DOTDASHPAY_RPCGEN_NODEJS_GENERATOR_H__
#define __DOTDASHPAY_RPCGEN_NODEJS_GENERATOR_H__

#include "generator_helpers.h"

#include <google/protobuf/descriptor.h>
#include <string>

namespace ddprpc_nodejs_generator {

// Contains all the parameters that are parsed from the command line.
struct Parameters {
};

// Return the prologue of the generated header file.
std::string GetPrologue(const google::protobuf::FileDescriptor *file, const Parameters &params);

// Return the includes for the generated source file.
std::string GetSourceIncludes(const google::protobuf::ServiceDescriptor* service, const Parameters &params);

// Return the implementation of the service with name.
std::string GetServiceImplementation(const google::protobuf::ServiceDescriptor* service, const Parameters &params);

// Return the simulator header.
std::string GetSimulatorHeader(const google::protobuf::FileDescriptor* file, const Parameters &params);

// Return the simulator implementation.
std::string GetSimulatorSource(const google::protobuf::FileDescriptor* file, const Parameters &params);

// Return the examples template.
std::string GetExamplesTemplate(const google::protobuf::FileDescriptor* file, const Parameters &params);

inline std::string GetClassPrefix() {
  return "";
}


}  // namespace ddprpc_nodejs_generator

#endif  // __DOTDASHPAY_RPCGEN_NODEJS_GENERATOR_H__
