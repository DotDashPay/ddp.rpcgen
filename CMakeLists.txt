cmake_minimum_required (VERSION 2.8.12)
project (dotdashpay.rpcgen C CXX)
# The version number.
set (dotdashpay.rpcgen_VERSION_MAJOR 0)
set (dotdashpay.rpcgen_VERSION_MINOR 1)

function(message)
  list(GET ARGV 0 MessageType)
  if(MessageType STREQUAL WARNING OR
      MessageType STREQUAL AUTHOR_WARNING)
  else()
    list(REMOVE_AT ARGV 0)
    _message(${MessageType} "${ARGV}")
  endif()
endfunction()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Standard configuration parameters
option(DEBUG_SYMBOLS "DEBUG_SYMBOLS" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

if(DEBUG_SYMBOLS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Find necessary packages
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}")

## This MUST go first to ensure the protos get converted into *.cc and
## *.h files before the rest of the modules are built.
set(ROOT_FIRMWARE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROTOBUF_IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/dotdashpay/api/common/protobuf")

file(GLOB PROTOBUF_FILES "${CMAKE_CURRENT_SOURCE_DIR}/dotdashpay/api/common/protobuf/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOBUF_FILES})
list(APPEND ALL_PROTOBUF_FILES ${PROTO_SRCS} ${PROTO_HDRS})
add_custom_target("compiled-cpp-protos" ALL DEPENDS ${ALL_PROTOBUF_FILES})

add_library("libapi" OBJECT "${CMAKE_BINARY_DIR}/dotdashpay/api/common/protobuf/api_common.pb.cc")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dotdashpay/rpcgen")
