include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(PLUGINS "cpp" "objc" "nodejs")

foreach(PLUGIN ${PLUGINS})
  add_executable(
    "ddprpc_${PLUGIN}_plugin"
    "${PLUGIN}_plugin.cc"
    "${PLUGIN}_generator.cc"
    "${CMAKE_BINARY_DIR}/dotdashpay/api/common/protobuf/api_common.pb.cc")
  target_link_libraries ("ddprpc_${PLUGIN}_plugin" ${PROTOBUF_PROTOC_LIBRARIES} ${PROTOBUF_LIBRARIES})
  add_dependencies("ddprpc_${PLUGIN}_plugin" compiled-cpp-protos)
endforeach()
