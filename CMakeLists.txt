# 设置cmake的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
project(corpc)

# 生成debug版本，可以进行gdb调试
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable")

# 设置项目可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置项目库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

enable_language(ASM)
set(CMAKE_ASM_COMPILER ${CMAKE_CXX_COMPILER})

# 设置项目编译头文件搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR})
# include_directories(${PROJECT_SOURCE_DIR}/corpc)
# include_directories(${PROJECT_SOURCE_DIR}/corpc/common)
# include_directories(${PROJECT_SOURCE_DIR}/corpc/coroutine)
# include_directories(${PROJECT_SOURCE_DIR}/corpc/net)
# include_directories(${PROJECT_SOURCE_DIR}/corpc/net/http)
# include_directories(${PROJECT_SOURCE_DIR}/corpc/net/pb)
# include_directories(${PROJECT_SOURCE_DIR}/corpc/net/tcp)
# 设置项目库文件搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# uninstall target
add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
    TARGET "uninstall"
    POST_BUILD
    COMMENT "Uninstall files with install_manifest.txt"
    COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in
            install_manifest.txt to be uninstalled!
)

add_subdirectory(${PROJECT_NAME})
add_subdirectory(test)
