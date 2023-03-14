# corpc

基于Linux下C++环境实现的协程异步RPC框架，也可以作为网络库使用。

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

```bash
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

这样就在 `generator` 的上级目录下生成 `UserService` 项目，可以在项目中 `UserService/interface` 的子目录下进行接口处理逻辑的定义。通过以下命令启动服务：

```bash
cd UserService
./clean.sh
./build.sh
cd bin
./UserService ../conf/UserService.yml

# 也可以通过该脚本来启动一个守护进程
./run.sh bin/UserService

# 停止守护进程
./shutdown.sh bin/UserService
```

### 定义自己的网络服务

RPC服务和其他非RPC协议的服务，有诸多共同之处。因此，本框架不仅支持基本的RPC功能，如果用户有自定义的通信协议，本框架又可摇身一变成为高性能网络库。

若想定义自己的网络服务，需要继承并重写以下几个类：

- `CustomStruct`：数据类，用于封装自定义协议的字段
- `CustomCodeC`：编解码类，用于定义自定义协议的编解码逻辑（`encode()`、`decode()` 方法）
- `CustomDispatcher`：处理类，用于自定义协议的处理逻辑（`dispatch()` 方法）
- `CustomService`：抽象服务类，用于服务注册

在服务端启动之前，需要对自定义协议进行设置。首先需要在服务端的配置文件中，将服务端协议类型设置为 `custom` ：

```yaml
......

server:
  ip: 127.0.0.1
  port: 10001
  protocol: custom

......
```

然后在服务端启动前，设置好自定义协议的数据对象、编解码对象、处理对象（以下代码不符合C++语法，仅为示例）：

```C++
#include <string>
#include <memory>
#include <corpc/common/start.h>
#include "your_data.h"
#include "your_codec.h"
#include "your_dispatcher.h"
#include "your_service.h"

int main()
{
    std::string configFile = "file.yml";
    corpc::initConfig(configFile); // 初始化配置，configFile: 配置文件路径

    corpc::getServer()->setConnectionCallback(yourFunc); // 当连接状态发生变化的回调函数
    corpc::getServer()->setCustomCodeC(std::make_shared<YourCodeC>()); // 设置编解码对象
    corpc::getServer()->setCustomDispatcher(std::make_shared<YourDispatcher>()); // 设置处理对象
    corpc::getServer()->setCustomData([]() {  return std::make_shared<YourStruct>(); }); // 设置数据对象

    REGISTER_SERVICE(YourService); // 注册服务，用于服务发现

    corpc::startServer(); // 启动服务
    
    return 0;
}
```

客户端在收发数据前，也需要进行类似的设置。

```C++
#include <string>
#include <memory>
#include <vector>
#include <corpc/net/tcp/tcp_client.h>
#include <corpc/net/load_balance.h>
#include <corpc/net/register/zk_service_register.h>
#include "your_data.h"
#include "your_codec.h"
#include "your_dispatcher.h"

int main()
{
    corpc::AbstractServiceRegister::ptr center = std::make_shared<corpc::ZkServiceRegister>("127.0.0.1", 2181, 30000);
    std::vector<corpc::NetAddress::ptr> addrs = center->discoverService("ProxyService_Custom"); // 服务发现，注册的自定义协议服务名都有_Custom后缀
    corpc::LoadBalanceStrategy::ptr loadBalancer = corpc::LoadBalance::queryStrategy(corpc::LoadBalanceCategory::Random);
    corpc::IPAddress::ptr addr = loadBalancer->select(addrs, corpc::PbStruct());
    client = std::make_shared<corpc::TcpClient>(addr, corpc::Custom_Protocol);
    client->setCustomCodeC(std::make_shared<ChatServiceCodeC>());
    client->setCustomData([]() {  return std::make_shared<ChatServiceStruct>(); });

    YourStruct request;
    ...... // 自定义协议的其他字段
    request.protocolData = "your data";
    YourCodeC codec;

    // 编码并发送数据
    codec.encode(client->getConnection()->getOutBuffer(), &request);
    client->sendData();
    
    std::shared_ptr<corpc::CustomStruct> response;
    int ret = client->recvData(response); // 发送数据
    if (ret != 0) {
        exit(-1);
    }
    std::shared_ptr<YourStruct> temp = std::dynamic_pointer_cast<YourStruct>(data); // 接收数据
    if (!temp) {
        exit(-1);
    }
    std::string finalStr = temp->protocolData;
    ...... // 对收到数据的处理逻辑
    
    ...... // 客户端其他逻辑
    
    return 0;
}
```

## 示例文件

 `test` 文件夹下有测试HTTP、RPC服务的定义，以及调用RPC服务的各种方法。

自定义协议的使用示例见 [cochat](https://github.com/LoveBettygirl/cochat) 项目。

## TODO

- 项目可能还存在未知bug，需要修复，并修复在错误情况下会出现的bug，让错误提示对小白更友好（例如端口被占用等）
- 加入对thrift等协议数据的支持
- 加入对RPC长连接的支持
- 加入服务熔断、降级、限流等其他服务治理功能
