# 递归搜索目录的源文件
file(GLOB_RECURSE SRC_LIST *.cpp *.cc *.h *.hpp *.S)
add_library(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(corpc pthread yaml-cpp protobuf dl zookeeper_mt)

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
)
