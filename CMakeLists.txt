# 设置cmake的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
project(corpc)

# 生成debug版本，可以进行gdb调试
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 设置项目可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置项目库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置项目编译头文件搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/common)
include_directories(${PROJECT_SOURCE_DIR}/src/coroutine)
include_directories(${PROJECT_SOURCE_DIR}/src/net)
include_directories(${PROJECT_SOURCE_DIR}/src/net/http)
include_directories(${PROJECT_SOURCE_DIR}/src/net/pb)
include_directories(${PROJECT_SOURCE_DIR}/src/net/tcp)
# 设置项目库文件搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src)