"""This is the autogenerator script that converts the DDP API spec
into a languge-specific API implementation.

Example call:

protoc -I../../ -I/usr/local/include --plugin=protoc-gen-ddprpc=./generator.py --ddprpc_out=../../../ddp.api.node/lib/autogen/ ../../dotdashpay/api/common/protobuf/services.proto

While there is no way to pass an argument to the generator via protoc
(it does not allow this), you can set the following environment
variables to control how this program is executed:

== RPCGEN_DATA_FILE

This works in tandem with the RPCGEN_DEBUG_MODE variable. If set, the
generator will save the raw input from protoc to the file specified in
this variable. The debug mode can then be used to read this file
without the need to go through protoc, which makes it very hard to
debug programs.

== RPCGEN_DEBUG_MODE

Set the to 1 if you want to run this program in standalone mode. You
must first run the program using the above command line and then can
subsequently run the program directly to insert breakpoints.

A good practical way to do this is:
RPCGEN_DATA_FILE=/tmp/datafile.bin protoc ... --plugin=protoc-gen-ddprpc=./generator.py ...
RPCGEN_DATA_FILE=/tmp/datafile.bin RPCGEN_DEBUG_MODE=1 ./generator.py

"""

from abc import ABCMeta, abstractmethod
from google.protobuf.compiler import plugin_pb2 as plugin
from google.protobuf.descriptor_pb2 import DescriptorProto, EnumDescriptorProto
from jinja2 import Environment, FileSystemLoader
import json
import os
import pickle
import re
import sys

EXAMPLE_VALUES_FILENAME = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                       "..", "api", "common", "spec", "example-values.json")
TEMPLATES_DIR = os.path.dirname(os.path.realpath(__file__)) + os.path.sep + "templates"

# Python protobuf module parses the update/completion service options
# into the following unicode sequences
# TODO(cjrd) submit an issue to python protobuf github about this parsing error
RESPONSE_TYPE = {
    "\x8a\xb5\x18": "UPDATE",
    "\x92\xb5\x18": "COMPLETION"
}

def get_method_options(method):
    """
    ARE YOU SEEING DICT KEY ERRORS IN THIS METHOD?
    Make sure you have python protobuf package version 2.6.1
    `pip show protobuf` # to check your version
    `pip uninstall protobuf` # to remove the old value
    `pip install protobuf==2.6.1` # to install the needed verison
    """
    responses = []
    for resp in method.options._unknown_fields:
        responses.append({
            "type": RESPONSE_TYPE[resp[0]],
            "name": resp[1][1:].split(".")[-1]
        })
    return responses


def to_camel_case(snake_str):
    """to_camel_case is a utility method for converting strings to camel
    case.

    """
    components = snake_str.split('_')
    return components[0] + "".join(x.title() for x in components[1:])

class DDPGenerator:
    """DDPGenerator is an abstract class that is used to generate the API
    in a target language.

    A target language should be subclassed from this class and
    requires only three basic definitions:

    1. Define a class that implements the abstract methods in this class (mostly for file naming)
    2. Define the set of templates that exemplify the implementation of the target language
    3. A main method that looks like this:

    #!/usr/bin/env python
    import generator

    class NameOfGenerator:
    ...

    if __name__ == '__main__':
        generator.run(NameOfGenerator())

    For each abstract method that has a method signature
    {type}_{header|source}_name(s?), you must create a file
    ./templates/{language}.{type}.{header|source}.j2, which is a jinja
    template. For languages that don't use header files, the
    associated template does not need to be created.

    """
    __metaclass__ = ABCMeta

    def __init__(self):
        with open(EXAMPLE_VALUES_FILENAME) as examples_file:
            self.examples = json.load(examples_file)

    def find_arguments_proto_by_method_name(self, proto):
        """find_proto_by_name returns a DescriptorProto of the argument
        message to the method.

        This is meant to be 'installed' as a jinja filter.

        """
        return self.find_proto_by_name("{}Args".format(proto))

    def find_response_args_proto_by_response_name(self, proto):
        """find_response_args_proto_by_response_name
        returns a DescriptorProto of the proto field for the given response

        This is meant to be 'installed' as a jinja filter.
        """

        return self.find_proto_by_name(proto)

    def find_proto_by_name(self, proto):
        """find_proto_by_name returns a DescriptorProto of the named proto

        This is meant to be 'installed' as a jinja filter.

        """
        for proto_file in self.fileset.proto_file:
            for message in proto_file.message_type:
                if message.name == proto:
                    return message

    def find_services(self, fileset):
        """find_services returns of list of protos that define services in the
        fileset.

        The API is autogenerated from a set of service definitions. Thus,
        a single service defines some subset of the entire possible API
        requests that can be issued.

        @return a list of ServiceDescriptorProto objects that describe all
        of the services in the fileset.

        """
        services = []
        for pfile in fileset.proto_file:
            for service in pfile.service:
                services.append(service)
        return services

    def run(self):
        """run is the main method that kicks off the autogeneration.

        Subclasses should call this method directly from their __main
        routine.

        """
        data = None

        if "RPCGEN_DEBUG_MODE" in os.environ and "RPCGEN_DATA_FILE" in os.environ:
            with open(os.environ["RPCGEN_DATA_FILE"], "r") as outfile:
                data = outfile.read()
        else:
            # Read request message from stdin
            data = sys.stdin.read()
            if "RPCGEN_DATA_FILE" in os.environ:
                with open(os.environ["RPCGEN_DATA_FILE"], "wb") as outfile:
                    outfile.write(data)

        # Parse request
        self.fileset = plugin.CodeGeneratorRequest()
        self.fileset.ParseFromString(data)

        # Get all of the services defined in the fileset.
        services = self.find_services(self.fileset)
        if len(services) == 0:
            raise Exception("Unable to find any services in the input files")

        # For each service, output a file that defines the API requests
        # for the service.
        outputs = plugin.CodeGeneratorResponse()
        self.generate(services, outputs)

        # Serialise response message
        output = outputs.SerializeToString()

        # Write to stdout
        sys.stdout.write(output)


    @abstractmethod
    def language(self):
        """language should return the language that this generator
        implements

        """
        pass

    @abstractmethod
    def beautify(self, code):
        """beautify should return aa string that is a beautified version of
        that string according to the languages.

        """
        pass

    @abstractmethod
    def recase(self, variable):
        """recase should convert the variable's name to the correct case for the language.

        e.g. In javascript, a variable should be in camelCase

        """
        pass

    @abstractmethod
    def api_header_name(self, service):
        """api_header_name should return the name of the file will hold the
        generated content that defines the API.

        In some languages it is ok to return None here to indicate
        that there are no header files (e.g. Python, Javascript).

        """
        pass

    @abstractmethod
    def api_source_name(self, service):
        """api_source_name should return the name of the file will hold the
        generated content that implements the API.

        """
        pass

    @abstractmethod
    def simulator_header_name(self, services):
        """simulator_header_name should return the name of the file will hold
        the generated content that defines the simulator.

        Note that this method takes a list of services, not just one,
        because the simulator should be generated to handle all API
        requests across all services.

        In some languages it is ok to return None here to indicate
        that there are no header files (e.g. Python, Javascript).

        """
        pass

    @abstractmethod
    def simulator_source_name(self, services):
        """simulator_source_name should return the name of the file will hold
        the generated content that implements the simulator.

        Note that this method takes a list of services, not just one,
        because the simulator should be generated to handle all API
        requests across all services.

        The implementation should be limited to simply controlling the
        behavior of the simulator and not the logic of the simulator
        itself. That is handled in the server. Rather, the
        implementation should simply pass along configuration changes
        to the simulator and API requests via a 'simulator bridge' to
        the server via POST requests and handle the responses.

        """
        pass

    @abstractmethod
    def examples_source_name(self, method):
        """examples_source_name should return the name of the file that will
        hold the example for the method.

        The example template file can (and should) make sure of the
        DDPGenerator function get_example value_for_field() to
        correctly get an example value given a protobuf field name.

        """
        pass

    @abstractmethod
    def tests_source_name(self, method):
        """tests_source_name should return the name of the file that will
        hold the test for the method.

        The tests are derived from the example template file.
        """
        pass

    def render(self, environment, typename, filetype, **kwargs):
        """render returns the contents of a filename using the jinja environment and context.

        The template will be loaded automatically from the typename
        and the filetype. Typename should be one of {api, simulator,
        examples} and filetype should be one of {header, source}.

        The context should be a dict of objects available to the
        template. Typically this will just be the service or services
        object passed from the generate method .

        """
        template_filename = "{}.{}.{}.j2".format(self.language(), typename, filetype)
        return environment.get_template(template_filename).render(**kwargs)

    def generate(self, services, outputs):
        """generate is the main method of the generators.

        It takes an object outputs, which should be created via:
        outputs = plugin.CodeGeneratorResponse()

        """
        PATH = os.path.dirname(os.path.abspath(__file__))
        environment = Environment(
            autoescape=False,
            loader=FileSystemLoader(TEMPLATES_DIR),
            trim_blocks=True)
        environment.filters["lowercase_first_letter"] = \
            lambda content: content[0].lower() + content[1:]
        environment.filters["remove_package"] = \
            lambda content: content.split(".")[-1]
        environment.filters["get_method_options"] = \
            lambda content: get_method_options(content)
        environment.filters["get_example_value_for_field"] = self.get_example_value_for_field
        environment.filters["find_arguments_proto_by_method_name"] = self.find_arguments_proto_by_method_name
        environment.filters["find_response_args_proto_by_response_name"] = self.find_response_args_proto_by_response_name
        environment.filters["find_proto_by_name"] = self.find_proto_by_name
        environment.filters["recase"] = self.recase

        # Generate API files.
        for service in services:
            header_name = self.api_header_name(service)
            if header_name is not None:
                generated_header_descriptor = outputs.file.add()
                generated_header_descriptor.name = header_name
                generated_header_descriptor.content = self.beautify(
                    self.render(environment, "api", "header", service = service))

        for service in services:
            source_name = self.api_source_name(service)
            if source_name is not None:
                generated_source_descriptor = outputs.file.add()
                generated_source_descriptor.name = source_name
                generated_source_descriptor.content = self.beautify(
                    self.render(environment, "api", "source", service = service))

        # Generate simulator files.
        for service in services:
            header_name = self.simulator_header_name(service)
            if header_name is not None:
                generated_header_descriptor = outputs.file.add()
                generated_header_descriptor.name = header_name
                generated_header_descriptor.content = self.beautify(
                    self.render(environment, "simulator", "header", service = service))

        for service in services:
            source_name = self.simulator_source_name(service)
            if source_name is not None:
                generated_source_descriptor = outputs.file.add()
                generated_source_descriptor.name = source_name
                generated_source_descriptor.content = self.beautify(
                    self.render(environment, "simulator", "source", service = service))

        # Generate example files. Standalone example files are for
        # reading purposes. The singular file (which is appended with
        # ".reference" is meant to be passed to generate-reference.js
        # in the api/common/spec directory.
        re_example = re.compile(r"// ?@example(.*?)// ?@example-end\(\)\n?", re.DOTALL)
        re_reference = re.compile(r"// ?@reference(.*?)// ?@reference-end\(\)\n?", re.DOTALL)
        re_test = re.compile(r"// ?@test(.*?)// ?@test-end\(\)\n?", re.DOTALL)
        re_lib_setup = re.compile(ur"// ?@example-lib-setup(.*?)// ?@example-lib-setup-end\(\)\n?", re.DOTALL)
        re_singles = re.compile(ur"// ?@single\((.*?)\)(.*?)// ?@single-end\(\)\n?", re.DOTALL)
        re_markup = re.compile(r"// @.*\n?")

        reference_content = ""

        singles_lines = {}
        singles_dedup = {}

        for service in services:
            for method in service.method:
                source_name = self.examples_source_name(method.name)
                if source_name is None:
                    continue

                generated_source_descriptor = outputs.file.add()
                generated_source_descriptor.name = source_name

                content = self.render(environment, "examples", "source", method = method, service = service)
                example_content = re_markup.sub("", re_reference.sub("", re_test.sub("", content)))
                generated_source_descriptor.content = self.beautify(example_content)

                reference_content += re_test.sub("", re_singles.sub("", content))

                singles = re_singles.findall(content)
                for single in singles:
                    identifier = single[0]
                    block = single[1]

                    if identifier not in singles_dedup:
                        singles_lines[identifier] = []
                        singles_dedup[identifier] = set()

                    for line in iter(block.splitlines()):
                        if line not in singles_lines[identifier]:
                            singles_lines[identifier].append(line)
                            singles_dedup[identifier].add(line)

                # Generate the test file as well
                source_name = self.tests_source_name(method.name)
                if source_name is None:
                    continue

                generated_source_descriptor = outputs.file.add()
                generated_source_descriptor.name = source_name
                test_content = re_markup.sub("", re_example.sub("", re_reference.sub("", content)))
                generated_source_descriptor.content = self.beautify(test_content)


        source_name = self.examples_source_name("reference")
        if source_name is not None:
            generated_source_descriptor = outputs.file.add()
            generated_source_descriptor.name = source_name

            reference_single_content = ""
            for identifier in singles_lines:
                reference_single_content += "// @{}(){}\n// @{}-end()\n\n".format(
                    identifier,
                    "\n".join(singles_lines[identifier]),
                    identifier)

            generated_source_descriptor.content = self.beautify(
                "{}\n\n{}".format(reference_single_content, reference_content))


    def _map_raw_example_value_to_language(self, raw_value):
        """_map_raw_example_value_to_language is used internally to convert a
        raw example value to a language-specific string.

        """
        language = self.language()

        has_language = False
        mapping = None
        if ("language_type_mappings" in self.examples) and (language in self.examples["language_type_mappings"]):
            has_language = True
            mapping = self.examples["language_type_mappings"][language]

        value = "";
        if isinstance(raw_value, basestring):
            # differentiate between enums and strings here
            if raw_value[:2] == "e:":
                value = "\"{}\"".format(raw_value.split(".")[-1])
            else:
                if has_language:
                    value = "{}{}{}".format(mapping["string_prefix"], raw_value, mapping["string_suffix"])
                else:
                    value = "\"{}\"".format(raw_value)
        elif isinstance(raw_value, bool):
            if has_language:
                value = mapping["true_value"] if raw_value else mapping["false_value"];
            else:
                value = "true" if raw_value else "false";
        elif isinstance(raw_value, list):
            if has_language:
                value = "{}{}{}".format(mapping["array_prefix"], raw_value, mapping["array_suffix"])
            else:
                value = "[{}]".format(raw_value)
        else:
            value = "{}".format(raw_value)

        return value

    def get_example_value_for_field(self, field_name):

        """get_example_value_for_field returns an example value as a string
        for a given language and protobuf field name.

        The set of possible example values is stored in a JSON file
        called example-values.json in dotdashpay/rpcgen.

        If the field_name is not found, this throws a runtime
        exception.

        """
        if not field_name in self.examples:
            raise Exception("Could not find an example value for field: {}".format(field_name))

        return self._map_raw_example_value_to_language(self.examples[field_name])
