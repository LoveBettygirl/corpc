# 递归搜索目录的源文件
file(GLOB_RECURSE SRC_LIST *.cpp *.cc *.h *.hpp *.S)
add_library(corpc ${SRC_LIST})
target_link_libraries(corpc pthread yaml-cpp protobuf dl zookeeper_mt)