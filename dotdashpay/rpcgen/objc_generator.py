#!/usr/bin/env python

from ddp_generator import DDPGenerator, to_camel_case
import jsbeautifier
import os
from subprocess import Popen, PIPE, STDOUT

UNCRUSTIFY_ROOT_DIR = "{}{}..{}..{}thirdparty{}uncrustify".format(os.path.dirname(os.path.realpath(__file__)),
                                                                  os.path.sep,
                                                                  os.path.sep,
                                                                  os.path.sep,
                                                                  os.path.sep)
UNCRUSTIFY_PATH = "{}{}src{}uncrustify".format(UNCRUSTIFY_ROOT_DIR, os.path.sep, os.path.sep)
UNCRUSTIFY_CFG = "{}{}configs{}objc.cfg".format(UNCRUSTIFY_ROOT_DIR, os.path.sep, os.path.sep)

class ObjectiveCGenerator(DDPGenerator):

    def class_prefix(self):
        return "DDP"

    def language(self):
        return "objc"

    def beautify(self, code):
        if not os.path.isfile(UNCRUSTIFY_PATH):
            raise "Cannot beautify Objective-C code without uncrustify, which can be built via setup.sh"

        pipe = Popen([UNCRUSTIFY_PATH, "-q", "-c", "{}".format(UNCRUSTIFY_CFG)],
                     stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        output = pipe.communicate(input="{}\n".format(code))[0]
        return output.decode()

    def recase(self, variable):
        return to_camel_case(variable)

    def api_header_name(self, service):
        return "{}{}.h".format(self.class_prefix(), service.name)

    def api_source_name(self, service):
        return None#"{}{}.m".format(self.class_prefix(), service.name)

    def simulator_header_name(self, services):
        return None

    def simulator_source_name(self, services):
        return None

    def examples_source_name(self, method_name):
        return None
        #return "{}Example.{}.m".format(self.class_prefix(), method_name)

    def tests_source_name(self, method_name):
        return None
        #return "{}Test.{}.m".format(self.class_prefix(), method_name)

if __name__ == '__main__':
    ObjectiveCGenerator().run()
