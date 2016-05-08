#!/usr/bin/env python

from ddp_generator import DDPGenerator, to_camel_case
from google.protobuf.descriptor_pb2 import FieldDescriptorProto
import jsbeautifier
import os
import re
from subprocess import Popen, PIPE, STDOUT

UNCRUSTIFY_ROOT_DIR = "{}{}..{}..{}thirdparty{}uncrustify".format(os.path.dirname(os.path.realpath(__file__)),
                                                                  os.path.sep,
                                                                  os.path.sep,
                                                                  os.path.sep,
                                                                  os.path.sep)
UNCRUSTIFY_PATH = "{}{}src{}uncrustify".format(UNCRUSTIFY_ROOT_DIR, os.path.sep, os.path.sep)
UNCRUSTIFY_CFG = "{}{}configs{}objc.cfg".format(UNCRUSTIFY_ROOT_DIR, os.path.sep, os.path.sep)

re_inline_comment = re.compile(ur"(.*;) *(//.*)(\n)(\n*)")

class ObjectiveCGenerator(DDPGenerator):

    def class_prefix(self):
        return "DDP"

    def import_from_proto_file(self, proto_path):
        filename = proto_path.split("/")[-1].replace(".proto", ".pbobjc.h")
        return filename[0].upper() + filename[1:]

    def language(self):
        return "objc"

    def beautify(self, code):
        if not os.path.isfile(UNCRUSTIFY_PATH):
            raise "Cannot beautify Objective-C code without uncrustify, which can be built via setup.sh"

        pipe = Popen([UNCRUSTIFY_PATH, "-q", "-l", "oc", "-c", "{}".format(UNCRUSTIFY_CFG)],
                     stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        output = pipe.communicate(input="{}\n".format(code))[0]
        intermediate = output.decode()

        # Fix annoying bug that doesn't keep spacing between code and
        # inline comment.
        fixed = re.sub(re_inline_comment, u"\g<1> \g<2>\n", intermediate)
        return fixed

    def get_type_name(self, protobuf_type):
        return {
            FieldDescriptorProto.TYPE_BOOL: "BOOL",
            FieldDescriptorProto.TYPE_BYTES: "NSData*",
            FieldDescriptorProto.TYPE_DOUBLE: "double",
            FieldDescriptorProto.TYPE_ENUM: "int32_t",
            FieldDescriptorProto.TYPE_FIXED32: "",
            FieldDescriptorProto.TYPE_FIXED64: "",
            FieldDescriptorProto.TYPE_FLOAT: "float",
            FieldDescriptorProto.TYPE_GROUP: "",
            FieldDescriptorProto.TYPE_INT32: "int32_t",
            FieldDescriptorProto.TYPE_INT64: "int64_t",
            FieldDescriptorProto.TYPE_MESSAGE: "",
            FieldDescriptorProto.TYPE_SFIXED32: "int32_t",
            FieldDescriptorProto.TYPE_SFIXED64: "int64_t",
            FieldDescriptorProto.TYPE_SINT32: "int32_t",
            FieldDescriptorProto.TYPE_SINT64: "int64_t",
            FieldDescriptorProto.TYPE_STRING: "NSString*",
            FieldDescriptorProto.TYPE_UINT32: "uint32_t",
            FieldDescriptorProto.TYPE_UINT64: "uint64_t",
        }[protobuf_type]

    def recase(self, variable):
        return to_camel_case(variable)

    def api_header_name(self, service):
        return None#return "{}{}.h".format(self.class_prefix(), service.name)

    def api_source_name(self, service):
        return None#return "{}{}.m".format(self.class_prefix(), service.name)

    def simulator_header_name(self, services):
        return None

    def simulator_source_name(self, services):
        return None

    def examples_source_name(self, method_name):
        return "{}Example.{}.m".format(self.class_prefix(), method_name)

    def tests_source_name(self, method_name):
        return None
        #return "{}Test.{}.m".format(self.class_prefix(), method_name)

if __name__ == '__main__':
    ObjectiveCGenerator().run()
