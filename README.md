Group-based Collective Operations for UCX

# 概要
UCG是对UCX的扩展，在其基础之上支持集合操作语义,旨在提升MPI集合操作性能。本分支的目的是重新设计UCG代码架构，简化对外接口，完善内部抽象，精细化资源利用，插件化算法实现，最小化UCX改动，充分考虑可读性、可扩展性、可维护性。

重写同时要实现的主要需求
1. 自动算法选择
2. 优化非连续数据类型

# 目录结构
```
├── api                               # 放置对外的头文件
│   ├── ucg.h                             # 对外的数据结构定义以及函数声明
│   ├── ucg_def.h                         # 类型别名
├── base                              # 放置内部基础功能的实现
│   ├── ucg_topo.c                        # 进程组拓扑信息
│   └── ...
├── core                              # 放置对外函数的实现
│   ├── ucg_request.c                     # 集合操作请求
│   └── ...                     
└── planner                           # 放置集合操作的底层实现，支持多组件
    ├── builtin                           # 内置的集合操作实现
    └── ...                           
```

# 抽象概念
## 对外
### RTE
RTE（runtime environment）是UCG运行依赖的外部信息的抽象，以运行于MPI环境为例，包含MPI的特定函数、MPI的特定常量等。进程需在使用UCG前初始化一次RTE，并在初始化时传入相关信息以保证UCG功能正常运行。单实例

### Context
Context是UCG运行上下文的抽象，进程内可存在多个实例，每个实例可独立配置。
* RTE:Context = 1:N

### Group
Group是通信组的抽象，只有在Group内才能执行集合操作，集合操作可使用的资源由其依托的Context决定。
* Context:Group = 1:N

### Request
Request是指定参数的集合操作的抽象，支持初始化一次、执行多次
* Group:Request = 1:N

## 对内
### Plan Pool
所有Plan会注册到Plan Pool中，对外提供Plan选择功能。单实例
* Request:Plans = N:1

### Channel
提供dst-id收发数据、注册am handler的功能，内部隐藏调用UCP创建EP、调用UCT收发数据的实现、链路复用、am handler未注册的处理等等。

### Topology
对上（Plan）提供查询进程间距离的功能，内部隐藏拓扑的管理结构。单实例

### Plan
算法生成的针对集合操作的通讯树，规定通讯步骤、每个步骤的通讯方向和数据大小。

## 抽象关系
### 从下到上
1. 由特定算法生成Plan，Plan注册到Plan Pool中

### 从上到下
1. 初始化RTE：保存外部指定的MPI特定函数和常量（常量也可通过函数方式获取）、全局拓扑信息
2. 创建Context：保存外部配置的Plan Name（默认为ALL）
3. 创建Group
3. 创建Request：根据Plan Name到Plan Pool中选择最佳的Plan，指定Request按该Plan执行
4. 执行Request：Request按照Plan规定的Action依次执行，并记录当前执行步骤。
