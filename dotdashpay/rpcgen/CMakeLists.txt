include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ddprpc_cpp_plugin 
  "cpp_plugin.cc" "cpp_generator.cc" "${CMAKE_BINARY_DIR}/dotdashpay/api/common/protobuf/api_common.pb.cc")
target_link_libraries (ddprpc_cpp_plugin ${PROTOBUF_PROTOC_LIBRARIES} ${PROTOBUF_LIBRARIES})
add_dependencies(ddprpc_cpp_plugin compiled-cpp-protos)
