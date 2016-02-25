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

#ifndef __DOTDASHPAY_RPCGEN_GENERATOR_HELPERS_H__
#define __DOTDASHPAY_RPCGEN_GENERATOR_HELPERS_H__

#include <dotdashpay/api/common/protobuf/api_common.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ddprpc_generator {

inline bool StripSuffix(std::string *filename, const std::string &suffix) {
  if (filename->length() >= suffix.length()) {
    size_t suffix_pos = filename->length() - suffix.length();
    if (filename->compare(suffix_pos, std::string::npos, suffix) == 0) {
      filename->resize(filename->size() - suffix.size());
      return true;
    }
  }

  return false;
}

inline std::string StripProto(std::string filename) {
  if (!StripSuffix(&filename, ".protodevel")) {
    StripSuffix(&filename, ".proto");
  }
  return filename;
}

inline std::string StringReplace(std::string str, const std::string &from,
                                  const std::string &to, bool replace_all) {
  size_t pos = 0;

  do {
    pos = str.find(from, pos);
    if (pos == std::string::npos) {
      break;
    }
    str.replace(pos, from.length(), to);
    pos += to.length();
  } while(replace_all);

  return str;
}

inline std::string StringReplace(std::string str, const std::string &from,
                                  const std::string &to) {
  return StringReplace(str, from, to, true);
}

inline std::vector<std::string> tokenize(const std::string &input,
                                          const std::string &delimiters) {
  std::vector<std::string> tokens;
  size_t pos, last_pos = 0;

  for (;;) {
    bool done = false;
    pos = input.find_first_of(delimiters, last_pos);
    if (pos == std::string::npos) {
      done = true;
      pos = input.length();
    }

    tokens.push_back(input.substr(last_pos, pos - last_pos));
    if (done) return tokens;

    last_pos = pos + 1;
  }
}

inline std::string CapitalizeFirstLetter(std::string s) {
  if (s.empty()) {
    return s;
  }
  s[0] = ::toupper(s[0]);
  return s;
}

inline std::string LowercaseFirstLetter(std::string s) {
  if (s.empty()) {
    return s;
  }
  s[0] = ::tolower(s[0]);
  return s;
}

inline std::string LowerUnderscoreToUpperCamel(std::string str) {
  std::vector<std::string> tokens = tokenize(str, "_");
  std::string result = "";
  for (unsigned int i = 0; i < tokens.size(); i++) {
    result += CapitalizeFirstLetter(tokens[i]);
  }
  return result;
}

inline std::string FileNameInUpperCamel(const google::protobuf::FileDescriptor *file,
                                         bool include_package_path) {
  std::vector<std::string> tokens = tokenize(StripProto(file->name()), "/");
  std::string result = "";
  if (include_package_path) {
    for (unsigned int i = 0; i < tokens.size() - 1; i++) {
      result += tokens[i] + "/";
    }
  }
  result += LowerUnderscoreToUpperCamel(tokens.back());
  return result;
}

inline std::string FileNameInUpperCamel(const google::protobuf::FileDescriptor *file) {
  return FileNameInUpperCamel(file, true);
}

enum MethodType {
  METHODTYPE_NO_STREAMING,
  METHODTYPE_CLIENT_STREAMING,
  METHODTYPE_SERVER_STREAMING,
  METHODTYPE_BIDI_STREAMING
};

inline MethodType GetMethodType(const google::protobuf::MethodDescriptor *method) {
  if (method->client_streaming()) {
    if (method->server_streaming()) {
      return METHODTYPE_BIDI_STREAMING;
    } else {
      return METHODTYPE_CLIENT_STREAMING;
    }
  } else {
    if (method->server_streaming()) {
      return METHODTYPE_SERVER_STREAMING;
    } else {
      return METHODTYPE_NO_STREAMING;
    }
  }
}

inline bool IsConformant(const google::protobuf::FileDescriptor* file, std::string* error) {
  for (int i = 0; i < file->service_count(); ++i) {
    for (int j = 0; j < file->service(i)->method_count(); ++j) {
      const google::protobuf::MethodDescriptor* method = file->service(i)->method(j);
      if (!method->options().HasExtension(dotdashpay::api::common::completion_response)) {
        (*error) = "Service [" + file->service(i)->name() + "] does not contain a completion response";
        return false;
      }
    }
  }

  return true;
}

inline std::vector<std::string> GetUpdateResponses(
    const google::protobuf::MethodDescriptor* method, const bool& get_package = false) {
  std::vector<std::string> responses;
  const int index = get_package ? 0 : 1;
  for (int i = 0; i < method->options().ExtensionSize(dotdashpay::api::common::update_response); ++i) {
    const std::string response_name = method->options().GetExtension(dotdashpay::api::common::update_response, i);
    responses.push_back(tokenize(response_name, ".")[index]);
  }
  return responses;
}

inline std::string GetCompletionResponse(
    const google::protobuf::MethodDescriptor* method, bool get_package = false) {
  const int index = get_package ? 0 : 1;
  const std::string response_name = method->options().GetExtension(dotdashpay::api::common::completion_response);
  return tokenize(response_name, ".")[index];
}

inline std::set<std::string> GetUniqueResponses(const google::protobuf::ServiceDescriptor* service,
                                                const bool& get_package = false) {
  std::set<std::string> responses;
  for (int j = 0; j < service->method_count(); ++j) {
    const google::protobuf::MethodDescriptor* method = service->method(j);
    const std::vector<std::string> method_responses = ddprpc_generator::GetUpdateResponses(method, get_package);
    responses.insert(method_responses.begin(), method_responses.end());
    responses.insert(ddprpc_generator::GetCompletionResponse(method, get_package));
  }
  return responses;
}

inline std::set<std::string> GetUniqueResponses(const google::protobuf::FileDescriptor* file,
                                                const bool& get_package = false) {
  std::set<std::string> responses;
  for (int i = 0; i < file->service_count(); ++i) {
    const std::set<std::string> service_responses = GetUniqueResponses(file->service(i), get_package);
    responses.insert(service_responses.begin(), service_responses.end());
  }
  return responses;
}

inline const google::protobuf::Descriptor* FindMessageByName(const google::protobuf::FileDescriptor* file,
                                                             const std::string& messageName) {
  const google::protobuf::Descriptor* response = file->FindMessageTypeByName(messageName);
  if (response == NULL) {
    for (int i = 0; i < file->dependency_count(); ++i) {
      const google::protobuf::FileDescriptor* dep = file->dependency(i);
      response = dep->FindMessageTypeByName(messageName);
      if (response != NULL) {
        break;
      }
    }
  }

  return response;
}

}  // namespace ddprpc_generator

#endif  // __DOTDASHPAY_RPCGEN_GENERATOR_HELPERS_H__
