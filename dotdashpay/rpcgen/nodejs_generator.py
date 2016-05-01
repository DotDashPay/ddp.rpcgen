#!/usr/bin/env python

from ddp_generator import DDPGenerator
import generator

class NodeJsGenerator(DDPGenerator):
    def language(self):
        return "nodejs"

    def api_header_name(self, service):
        return None

    def api_source_name(self, service):
        return service.name.lower() + ".js"

    def simulator_header_name(self, services):
        pass

    def simulator_source_name(self, services):
        return "simulator.js"

    def examples_source_name(self, method_name):
        return "../../examples/" + method_name + ".example.js"

if __name__ == '__main__':
    NodeJsGenerator().run()
