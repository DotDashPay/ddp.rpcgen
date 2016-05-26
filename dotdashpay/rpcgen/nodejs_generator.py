#!/usr/bin/env python

from ddp_generator import DDPGenerator, to_camel_case
import jsbeautifier

class NodeJsGenerator(DDPGenerator):
    def language(self):
        return "nodejs"

    def beautify(self, code):
        opts = jsbeautifier.default_options()
        opts.indent_size = 2
        opts.brace_style = "collapse-preserve-inline"
        return jsbeautifier.beautify(code, opts)

    def get_type_name(self, protobuf_type):
        return "var"

    def recase(self, variable):
        return to_camel_case(variable)

    def api_header_name(self, service):
        return None

    def api_source_name(self, service):
        return service.name.lower() + ".js"

    def simulator_header_name(self, services):
        return None

    def simulator_source_name(self, services):
        return None

    def examples_source_name(self, method_name):
        return method_name + ".example.js"

    def tests_source_name(self, method_name):
        return method_name + ".test.js"

if __name__ == '__main__':
    NodeJsGenerator().run()
