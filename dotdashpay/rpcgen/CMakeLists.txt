include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(PLUGINS "cpp" "objc" "nodejs")

foreach(PLUGIN ${PLUGINS})
  add_executable(
    "ddprpc_${PLUGIN}_plugin"
    "${PLUGIN}_plugin.cc"
    "${PLUGIN}_generator.cc"
    $<TARGET_OBJECTS:libapi>)
  target_link_libraries ("ddprpc_${PLUGIN}_plugin" ${PROTOBUF_PROTOC_LIBRARIES} ${PROTOBUF_LIBRARIES})
  add_dependencies("ddprpc_${PLUGIN}_plugin" compiled-cpp-protos)
endforeach()
