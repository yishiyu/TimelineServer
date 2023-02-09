# TimelineServer

使用C++实现的Web服务器,代码规范参考的是[Google 命名约定](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/naming/)

## 功能

主体框架参照了[WebServer](https://github.com/markparticle/WebServer)项目,同时增加了以下功能:

- 基于注册的静态请求跳转(如"/"跳转到"/index.html")
- 基于回调函数的动态请求处理(登录/数据库增删改查)
- POST请求解析(原项目做了"urlencoded"格式请求体的解析,改成了"json"格式的解析)
- 基于"MySQL Connector/C++"的数据库系统(原项目使用的是C的数据库函数)
- 利用"gtest"为每一个模块都写了模块测试

## 安装环境与启动

- 安装依赖库

    ```shell
    sudo apt update

    # 安装 mysql connector
    sudo apt install libmysqlclient-dev

    # 安装 json11
    sudo apt install libjson11-1-dev
    ```

- 编译

    ```shell
    git clone https://github.com/yishiyu/TimelineServer.git
    bash ./build.sh
    ```

- 配置 mysql

    ```shell
    # 安装服务器
    sudo apt install mysql-server

    # 安装客户端
    sudo apt install mysql-client

    # 配置用户账号密码和代码中对应
    # 默认为 root explosion

    # 创建数据库
    CREATE DATABASE timelineserver;
    CREATE TABLE IF NOT EXISTS users(
        user_id BIGINT(20) UNSIGNED AUTO_INCREMENT,
        user_name VARCHAR(40) NOT NULL,
        user_passwd VARCHAR(40) NOT NULL,
        PRIMARY KEY (user_id)
    );
    CREATE TABLE IF NOT EXISTS tasks(
        task_id BIGINT(20) UNSIGNED AUTO_INCREMENT,
        user_id BIGINT(20) UNSIGNED NOT NULL,
        time DATE NOT NULL,
        task VARCHAR(300) NOT NULL,
        priority INT DEFAULT 0 NOT NULL,
        PRIMARY KEY (task_id)
    );
    ```

- 获取资源文件

    ```shell
    git submodule update --init --recursive
    mv TimeLineFrontend data/resources/
    ```

- 启动

    ```shell
    # cd 到 build 目录
    ./TimelineServer
    ```

## 项目结构

```text
.
├── build                           // 构建目录
├── build.sh                        // 构建脚本
├── CMakeLists.txt
├── code                            // 源代码
│   ├── buffer
│   ├── http
│   ├── log
│   ├── mux                         // IO 多路复用
│   ├── pool
│   ├── server
│   ├── timer
│   ├── main.cc                     // 代码入口
│   ├── routers.cc
│   └── routers.h                   // 路由函数
├── data
│   ├── log                         // 运行日志
│   ├── resources                   // 静态资源文件
│   └── test
│       ├── buffer
│       │   └── buffer_read.txt             // Buffer 读文件测试文件
│       ├── http
│       │   ├── conn_request_fail.txt       // HttpConn 访问不存在文件的报文
│       │   ├── conn_request_success.txt    // HttpConn 访问存在文件的报文
│       │   ├── request_get.txt             // HttpRequest 解析 Get 报文
│       │   ├── request_post.txt            // HttpRequest 解析的 Post 报文
│       │   └── response_resource.txt       // 测试用的资源文件
│       └── log
├── test                            // 对应模块的单元测试
│   ├── buffer
│   ├── http
│   ├── log
│   ├── mux
│   ├── pool
│   ├── server
│   └── timer
└── TimeLineFrontend                // 前端资源项目
```

## 前后端交互逻辑

- 事件描述:

    ```json
    {   
        // 由服务器生成的唯一id
        "task_id": "xxx",
        "user_id": "xxx",
        "time": "时间戳",
        "task": "xxx",
        "priority": "0-10(越大越重要)"
    }
    ```

- 服务器交互
  - 登陆之后,服务器回复浏览器一个 `action_token` 作为浏览器的身份凭证
  - 浏览器断开链接/主动退出登录后 `action_token` 销毁

  - 浏览器发送内容

    ```json
    {
        "action": "login/logout/query/add/update/delete",
        "action_info": {
            <!-- action 为 xxx 时需要的信息 -->
            <!-- login: -->
            "user": "xxx",
            "passwd": "xxx",

            <!-- logout: -->
            "action_token": "xxx",

            <!-- query: -->
            "action_token": "xxx",

            <!-- add: -->
            "action_token": "xxx",
            "time": "时间戳",
            "task": "xxx",
            "priority": "0-10(越大越重要)",

            <!-- update: -->
            "action_token": "xxx",
            "task_id": "xxx",
            "time": "时间戳",
            "task": "xxx",
            "priority": "0-10(越大越重要)",
            
            <!-- delete -->
            "action_token": "xxx",
            "task_id": "xxx", 
        },
    }
    ```

  - 服务器回复内容

      ```json
      {
          "action_result": "true/false",
          "result_info": {
              // login: 
              "action_token": "xxx",
              // add:
              // task id 是数据库自动生成的,不太好返回,在add之后重新执行一下querry算了
              // "task_id": "xxx",
              // query:
              "tasks": [
                {
                  "task_id": "xxx",
                  "time": "时间戳",
                  "task": "xxx",
                  "priority": "0-10(越大越重要)"
                }
              ]
              
              // 发生错误时
              "error_message" :"xxx",
          }
      }
      ```

## 测试方法

### VSCode 设置(launch.json)

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Server Debug",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/TimelineServer",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "为 gdb 启用整齐打印",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "将反汇编风格设置为 Intel",
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```

### 浏览器调试语句

- login 动态路由

    ```js
    fetch(new Request('action/login',{
        method:'POST', 
        headers: {'Content-Type': 'application/json'},
        body:"{\"action\":\"login\", \"action_info\": {\"user\":\"user\", \"passwd\": \"passwd\"}}"
    })).then((resp)=>{console.log(resp)})
    ```

- logout 动态路由

    ```js
    fetch(new Request('action/logout',{
        method:'POST', 
        headers: {'Content-Type': 'application/json'},
        body:"{\"action\":\"logout\", \"action_info\": {\"action_token\":\"PKdhtXMmr29n3L0K99eM\"}}"
    })).then((resp)=>{console.log(resp)})
    ```

- query 动态路由

    ```js
    fetch(new Request('action/query',{
        method:'POST', 
        headers: {'Content-Type': 'application/json'},
        body:"{\"action\":\"query\", \"action_info\": {\"action_token\":\"PKdhtXMmr29n3L0K99eM\"}}"
    })).then((resp)=>{console.log(resp)})
    ```

- add 动态路由

    ```js
    fetch(new Request('action/add',{
        method:'POST', 
        headers: {'Content-Type': 'application/json'},
        body:"{\"action\":\"add\", \"action_info\": {\"action_token\":\"PKdhtXMmr29n3L0K99eM\",\"time\":\"2024-01-01\",\"task\":\"test task\",\"priority\": 3}}"
    })).then((resp)=>{console.log(resp)})
    ```

- update 动态路由

    ```js
    fetch(new Request('action/update',{
        method:'POST', 
        headers: {'Content-Type': 'application/json'},
        body:"{\"action\":\"update\", \"action_info\": {\"action_token\":\"PKdhtXMmr29n3L0K99eM\",\"task_id\": 4, \"time\":\"2099-01-01\",\"task\":\"test task\",\"priority\": 3}}"
    })).then((resp)=>{console.log(resp)})
    ```

- delete 动态路由

    ```js
    fetch(new Request('action/delete',{
        method:'POST', 
        headers: {'Content-Type': 'application/json'},
        body:"{\"action\":\"delete\", \"action_info\": {\"action_token\":\"PKdhtXMmr29n3L0K99eM\",\"task_id\": 9}}"
    })).then((resp)=>{console.log(resp)})
    ```
