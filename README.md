# corpc

基于Linux下C++环境实现的协程异步RPC框架。

## Features

  - 搭建**协程模块**，实现协程创建、协程切换等功能，并提供给外部用户协程接口。将协程和Epoll结合实现M+N协程调度，达到用同步的方式实现异步的性能。
  - 并发模型采用Epoll LT+**主从Reactor**+非阻塞I/O+线程池，使用轮询方式将新连接分给其中一个Reactor线程。
  - 基于**时间轮**算法，以O(1)的效率高效检测并**关闭超时连接**，减少无效连接资源的占用。
  - 基于C++ stringstream实现了**异步日志系统**，并分开存放框架日志和业务日志，在不影响服务器运行效率的同时记录服务器的运行状态，便于排查问题。
  - 利用timerfd提供毫秒级精度的定时器，为时间轮和异步日志系统提供支持。
  - 同时实现了基本的HTTP协议通信，以及基于Protobuf的序列化和反序列化，并基于Protobuf实现了自定义的通信协议，来传输序列化后的RPC调用数据。
  - 借助ZooKeeper提供的服务治理功能，实现了框架的服务注册和服务发现功能。
  - 实现了随机、轮询、一致性哈希**三种负载均衡策略**，并使用**简单工厂模式**进行封装。
  - 在RPC客户端实现了简单的**RPC调用异常重试**的机制，提升了框架的**容错性**。
  - 实现**项目生成脚本**，可由Protobuf文件**一键生成项目**，助力用户使用本框架快速搭建高性能RPC服务。
  - 使用框架搭建HTTP回显服务，经wrk压力测试（4线程，1万并发连接），在单机可达到**2万以上的QPS**。

## 依赖

- gcc v7.5
- CMake（>=3.0）
- Protobuf
- yaml-cpp
- ZooKeeper（>=3.4，需要先安装JDK）

## 项目使用

### 构建和安装项目

安装完依赖之后，使用如下命令构建并安装项目：

```
./clean.sh
./build.sh
sudo ./install.sh
```

### 定义并生成RPC服务

定义Protobuf文件 `UserService.proto`：

```protobuf
syntax = "proto3";

option cc_generic_services = true;

message LoginRequest {
    int32 user_id = 1;
    bytes user_password = 2;
    bytes auth_info = 3;
}

message LoginResponse {
    int32 ret_code = 1;
    string res_info = 2;
}

message RegisterRequest {
    bytes user_name = 1;
    bytes user_password = 2;
}

message RegisterResponse {
    int32 ret_code = 1;
    string res_info = 2;
    int32 user_id = 3;
}

message LogoutRequest {
    int32 user_id = 1;
}

message LogoutResponse {
    int32 ret_code = 1;
    string res_info = 2;
}

service UserServiceRpc {
    rpc Login(LoginRequest) returns(LoginResponse);
    rpc Register(RegisterRequest) returns(RegisterResponse);
    rpc Logout(LogoutRequest) returns(LogoutResponse);
}
```

一键生成RPC服务：

```bash
cd generator
python corpc_gen.py -i ../UserService.proto -o ../
```

这样就在 `generator` 的上级目录下生成 `UserService` 项目，可以在项目中 `UserService/interface` 的子目录下进行接口逻辑的定义。通过以下命令启动服务：

```
cd UserService
./clean.sh
./build.sh
cd bin
./UserService ../conf/UserService.yml
```

## TODO

- 优化非RPC自定义协议在框架中的封装，让它用起来更像个网络库（高优先级）
- 加入服务熔断、降级、限流等其他服务治理功能（低优先级）
