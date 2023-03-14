find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 设置需要编译的源文件列表
set(TEST_COROUTINE ./test_coroutine.cpp)
set(TEST_HTTP_SERVER ./test_http_server.cpp)
set(TEST_PB_SERVER ./test_pb_server.cpp)
set(TEST_PB_SERVER_CLIENT ./test_pb_server_client.cpp)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test_pb_server.proto)

# 设置可执行文件生成路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(test_coroutine ${TEST_COROUTINE})
target_link_libraries(test_coroutine ${PROJECT_NAME} pthread zookeeper_mt)

add_executable(test_http_server ${TEST_HTTP_SERVER} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_http_server ${PROJECT_NAME} pthread ${Protobuf_LIBRARIES} zookeeper_mt)

add_executable(test_pb_server ${TEST_PB_SERVER} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_pb_server ${PROJECT_NAME} pthread ${Protobuf_LIBRARIES} zookeeper_mt)

add_executable(test_pb_server_client ${TEST_PB_SERVER_CLIENT} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_pb_server_client ${PROJECT_NAME} pthread ${Protobuf_LIBRARIES} zookeeper_mt)
