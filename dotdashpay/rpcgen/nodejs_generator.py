#!/usr/bin/env python
"""
This file generates a large portion of the node api

Example call:

protoc -I../../ -I/usr/local/include --plugin=protoc-gen-ddprpc=./nodejs_generator.py --ddprpc_out=../../../ddp.api.node/lib/autogen/ ../../dotdashpay/api/common/protobuf/services.proto

"""


from google.protobuf.compiler import plugin_pb2 as plugin
from google.protobuf.descriptor_pb2 import DescriptorProto, EnumDescriptorProto
from jinja2 import Environment, FileSystemLoader
import os
import sys


# Python protobuf module parses the update/completion service options
# into the following unicode sequences
# TODO(cjrd) submit an issue to python protobuf github about this parsing error
RESPONSE_TYPE = {
    "\x8a\xb5\x18": "UPDATE",
    "\x92\xb5\x18": "COMPLETION"
}

PATH = os.path.dirname(os.path.abspath(__file__))
TEMPLATE_ENVIRONMENT = Environment(
    autoescape=False,
    loader=FileSystemLoader(os.path.join(PATH, 'templates')),
    trim_blocks=True)
TEMPLATE_ENVIRONMENT.filters["lowercase_first_letter"] = \
  lambda method_name: method_name[0].lower() + method_name[1:]

def render_template(template_filename, context):
    return TEMPLATE_ENVIRONMENT.get_template(template_filename).render(context)


def parse_responses(req_options):
    """
    ARE YOU SEEING DICT KEY ERRORS IN THIS METHOD?
    Make sure you have python protobuf package version 2.6.1
    `pip show protobuf` # to check your version
    `pip uninstall protobuf` # to remove the old value
    `pip install protobuf==2.6.1` # to install the needed verison
    """
    responses = []
    for resp in req_options._unknown_fields:
        responses.append({
            "type": RESPONSE_TYPE[resp[0]],
            "name": resp[1][1:]
        })
    return responses


def create_message_name_to_descriptor(request):
    return_dict = {}
    for pfile in request.proto_file:
        for message_type in pfile.message_type:
            return_dict[message_type.name] = message_type
    return return_dict


def generate_code(request, response):
    """
    generate_code outputs one file for each api subsystem, e.g. hardware.js, payment.js, etc
    and one file for each request, e.g. ListenForPaymentDataThenSettle.js
    which defines the request object.
    """
    message_name_to_descriptor = create_message_name_to_descriptor(request)

    # find the service file object
    services = None
    for pfile in request.proto_file:
        if pfile.name == "dotdashpay/api/common/protobuf/services.proto":
            services = pfile
            break

    if services is None:
        raise Exception("Unable to find services file")

    for system in services.service:
        # system is payment, global, hardware, etc

        # create a request object file for each rpc
        requests = []
        for rpc in system.method:
            input_arg_name = rpc.input_type.split(".")[-1]
            requests.append({
                "name": rpc.name,
                "argument": message_name_to_descriptor[input_arg_name]
                })

            responses = parse_responses(rpc.options)
            for resp in responses:
                resp["name"] = resp["name"].split(".")[-1]

            rpc_dict = {
                "responses": responses,
                "rpc_name": rpc.name,
                "rpc_input": rpc.input_type
            }

            request_object_file = response.file.add()
            request_object_file.name = rpc.name + "Request.js"
            request_object_file.content = render_template("nodejs-request-object.js.j2", rpc_dict)

        # create the public api for each system
        public_api_system_file = response.file.add()
        public_api_system_file.name = system.name.lower() + ".js"
        requests.sort(key=lambda k: k["name"])
        public_api_system_file.content = render_template("nodejs-api-namespace.js.j2", {"requests": requests})


if __name__ == '__main__':
    # Read request message from stdin
    data = sys.stdin.read()

    # The below code is useful for quickly developing/testing
    # with open("tmp.out", "wb") as outfile:
    #     outfile.write(data)
    # with open("tmp.out", "r") as outfile:
    #     data = outfile.read()

    # Parse request
    request = plugin.CodeGeneratorRequest()
    request.ParseFromString(data)

    # Create response
    response = plugin.CodeGeneratorResponse()

    # Generate code
    generate_code(request, response)

    # Serialise response message
    output = response.SerializeToString()

    # Write to stdout
    sys.stdout.write(output)
