#!/usr/bin/env python

from ddp_generator import DDPGenerator
import jsbeautifier

class NodeJsGenerator(DDPGenerator):
    def language(self):
        return "nodejs"

    def beautify(self, code):
        return jsbeautifier.beautify(code)

    def api_header_name(self, service):
        return None

    def api_source_name(self, service):
        return service.name.lower() + ".js"

    def simulator_header_name(self, services):
        return None

    def simulator_source_name(self, services):
        return None

    def examples_source_name(self, method_name):
        return "../../examples/" + method_name + ".example.js"

    def tests_source_name(self, method_name):
        return "../../tests/" + method_name + ".test.js"

if __name__ == '__main__':
    NodeJsGenerator().run()
