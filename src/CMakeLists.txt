# 递归搜索目录的源文件
file(GLOB_RECURSE SRC_LIST *.cpp *.h)
add_library(corpc ${SRC_LIST})
target_link_libraries(corpc pthread yaml-cpp protobuf)