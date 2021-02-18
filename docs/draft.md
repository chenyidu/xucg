# 前言
本文档记录在重新设计和实现UCG代码的过程中的一些想法。

目标
- 简化对外接口
- 完善内部抽象
- 减少资源占用
- 插件化集合算法
- 最小化UCX改动

此外要充分考虑可读性、可扩展性、可维护性，确定一些约束避免代码腐坏（约束需要尽可能通过编程技术来保证，实在不行则在显著位置使用注释）。

比较明确的需求
1. 最佳算法选择：在多个集合算法中选择最适用于当前条件（通讯成员、数据类型大小等）的算法
2. 支持非连续数据类型

# 目录结构
```
├── api                               # 对外头文件目录
│   ├── ucg.h                             # 对外的数据结构定义以及函数声明
│   ├── ucg_def.h                         # 类型别名
├── base                              # 内部基础功能目录
│   ├── ucg_topo.c                        # 进程组拓扑信息
│   └── ...
├── core                              # 对外功能实现目录
│   ├── ucg_request.c                     # 集合操作请求
│   └── ...                     
└── plans                             # 集合操作算法实现目录
    ├── recursive_doubling                # 以算法类型命名的目录
    |   ├── allreduce.c                     # 基于该类算法实现的Allreduce Plan
    |   ├── barrier.c                       # 基于该类算法实现的Barrier Plan  
    |   ├── rd.c                            # 算法实现
    |   ├── rd.h
    └── ...                           
```
目录结构与UCX其他组件的风格尽量保持一致，如api、core、base。
- UCX所有组件都使用api目录
- UCP使用了core目录，
- UCT使用了base目录：UCT组件的架构是所有特化的TL都依赖于底层的更为通用的TL，这些通用的TL实现所在目录就是base，在UCG中base目录也是用来存放core和plans依赖的功能实现。
- plans目录为UCG特有的目录，用于保存集合操作算法和其生成的Plan

# 抽象的简单定义
## 对外
### RTE
RTE（runtime environment）是UCG运行依赖的外部信息的抽象，以运行于MPI环境为例，包含MPI的特定函数、MPI的特定常量等。因为这些信息只需要初始化一次，所以专门定义一个抽象用来描述这些信息。
- 单实例

### Context
Context是集合操作的执行资源的抽象。不同Context可以指定不同的资源。
- RTE:Context = 1:N
> 目前集合操作的执行资源是Plan

### Group
Group是通信组的抽象，集合操作在Group内执行，集合操作可使用的资源由Group依托的Context决定。
- Context:Group = 1:N

### Request
Request是集合操作的抽象，支持初始化一次、执行多次
- Group:Request = 1:N

## 对内
### Plan Pool
所有Plan会注册到Plan Pool中，提供选择Plan功能（即最佳算法选择）
- 单实例

### Channel
Channel是通讯管道的抽象，提供收发数据功能，隐藏调用UCP创建EP、调用UCT收发数据、链路复用、am handler未注册的处理等等。
- 单实例

### Topology
提供查询进程间距离的功能，内部隐藏拓扑的管理结构。
- 单实例

### Plan
根据特定算法生成的针对某一特定集合操作的通讯步骤、以及每个步骤的通讯对象和数据大小等。

### Action
Action是Plan中通讯步骤的抽象，集合操作执行框架从Plan中依次取出Action并执行。

## 抽象间的关系
### 从下到上
1. 由特定算法生成Plan，Plan注册到Plan Pool中

### 从上到下
1. 初始化RTE：保存外部指定的MPI特定函数和常量（常量也可通过函数方式获取）、全局拓扑信息
2. 创建Context：保存外部配置的Plan Name（默认为ALL）
3. 创建Group：指定通讯组成员
3. 创建Request：根据可用的Plan Name到Plan Pool中选择最佳的Plan并实例化，指定Request按该Plan执行
4. 执行Request：框架执行Action并返回下一个Action，Request记录Plan的下一个Action，直到Action为UCG_LAST_ACTION时，标记Request状态为完成。

# 附录
围绕以下几个问题的思考
1. 设计中是否已经没有可以减少的部分？
2. 抽象之间的交互是否考虑清晰？
3. 每个抽象的功能是否定义清晰？

## Planner是否有存在的必要？—— 否
不同Planner包含着不同的Plan，一旦划分Planner，那么一个Planner中的Plan依赖于另一个Planner里的Plan是相当违反直觉的，但实际中一个新算法会依赖原先的一些基础算法，而基础算法通常都位于builtin Planner。因此就有了这个问题。

从用户角度看
1. 因为用户的诉求是在所有可用的Plan中选择最佳的Plan，所以让用户指定Planner是没有意义的，用户必然指定ALL。

从实现角度看
1. 原先Planner还包含了收发流程，如果不同Planner包含不同的收发流程，那么确实有必要存在Planner。但现在会将收发流程提炼出来，因此Planner将只包含Plan，就没有必要存在不同的Planner。
2. Plan之间本身就是隔离的，用不同的Planner包含不同的Plan，没什么意义。
3. 目前Planner的注册方式是通过UCS FRAMEWORK LOAD，支持动态加载，因此可将以二进制形式提供的Plan放在一个Planner中动态加载。这可以通过别的方式来实现而无需添加新的抽象，比如源码形式提供的Plan统一编译到libucg.so中，二进制形式提供的Plan可编译为libucg_ext.so，直接作为一个MODULE进行加载。

## Plan是否需要感知到Request？—— 否
Request可以通过Plan提供的接口去执行Plan，并将执行状态记录在Request中，因此Plan无需感知Request。Plan并不需要直接操作Request，并在其中记录特定的信息。

## Plan复用
在定义上，一个实例化的Plan是包含集合操作所有信息的，比如通讯对象、通讯数据。复用Plan实际就是复用Plan Action。Plan Action中的通讯对象只受算法影响的，通讯数据则受算法和集合操作本身的数据共同影响。在一个Group内的集合操作的通讯成员是固定的，也就是说同一算法的Plan Action的通讯对象是固定的，可以直接复用。通讯数据会变化，但确定每个Action通讯数据的算法是固定的。
```
Action
{
    type: Send/Recv/Reduce/Generic  Action类型
    peers: 0,1,2,3 对端ID
    data: NULL  初始为NULL，通过data function计算得到
    data function:
}
```
复用
- 若集合操作的参数完全一致，则可完整复用Plan
- 若集合操作的数据存在不同，则执行Plan Clone。Plan Clone会直接拷贝Action，并将data置为NULL。更近一步，将type、peers、data function定义为constant field，直接引用内存空间，减少拷贝。
```
constant field
{
    reference count
    type: Send/Recv/Reduce/Generic  Action类型
    peers: 0,1,2,3 对端ID
    data function:
}

Action
{
    constant field pointer ->
    data: NULL  初始为NULL，通过data function计算得到
}
```

Group间的Plan复用可能性较低，不在考虑范围内。
