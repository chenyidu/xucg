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
- plans目录为UCG特有的目录，用于保存集合操作算法和其生成的Plan。
- - 子目录以算法名命名，同类的优化算法可放在同一目录下，比如recursive_doubling下可以有RD、Topo-Aware RD等。

# 编码风格
使用UCX编码风格，除了对齐等号、变量，因为这样的做法由一个弊端是会在修改中增加一些无效操作，增加review成本，比如
```
a    = 0;
aaaa = 0;
```
新增一行时，前两行都需要重新调整。
```
a    ..= 0;
aaaa ..= 0; 
aaaaaa = 0;
```
> 后续有需要时，通过工具统一刷格式。

# 抽象
| 名字 | 描述 | 关系 | 备注 |
| --- | --- | --- | --- |
| RTE |Runtime Environment, UCG运行时所依赖的环境信息，以运行于MPI环境为例，包含MPI函数、MPI常量等。| 单例 | 由User在使用UCG前初始化一次，相关信息会被内部使用 |
| Context | 集合操作的执行上下文（执行资源） | RTE:Context = 1:N | 由User创建，可指定使用的Plan |
| Group | 集合操作通讯组 | Context:Group = 1:N | 由User创建，等同于MPI Communicator |
| Request | 集合操作 | Group:Request = 1:N | 由User创建，支持创建一次，执行多次。 |
| Plan | 生成集合操作通讯步骤的算法 | Request:Plan = 1:1 | 对外以Plan Name为标识供User指定 |
| Action | 算法行为，如收、发、规约等 | Plan:Action = 1:N | 内部抽象，Plan根据算法创建Action |
| Plan Pool | Plan管理者 | Context:Plan Pool = 1:1 | 内部抽象，提供选择Plan（最佳算法选择）功能 |
| Channels | 通讯管道管理 | 单例 | 内部抽象，提供收发数据功能，隐藏创建UCP EP、收发数据方式、链路复用、am handler注册等等。Channel指代UCP EP，因此不提供独立的抽象概念 | 
| Topology | 运行环境中所有成员（进程）的拓扑信息 | 单例 | 内部抽象，以MPI为例，保存MPI_COMM_WORLD内所有进程的拓扑信息 |

# 命名约定
| 名字 | 描述 | 备注 |
| --- | --- | --- |
| handle | User创建Group时指定的成员标识，要求全局唯一。全局唯一的意思是该成员在不同Group中使用同一个handle。 | 以MPI运行环境为例，handle可以是`ompi_proc_t*`或`MPI_COMM_WORLD rank`。 |
| rank | 成员在Group handles数组中的下标。 | 仅用于简化Plan Action的生成，当需要通讯时需转换为handle。 |

# 实现约定
| 行为 | 描述 | 备注 |
| --- | --- | --- |
| Request Init| 当创建Request，若集合操作存在root，那么需传入root rank而非root handle。 | 通过root rank可快速找到root handle，若传入root handle则生成Plan时需要遍历Group handles得到root rank。 |
| Plan Clone| 基于Copy On Write原则复制Plan。 | 1. 若集合操作完全一样，那么增加Plan引用计数后返回当前Plan。<br> 2. 若集合操作存在差异，那么创建Plan，但复用Action。 |
| Action | Action只代表一类操作，不能出现RECV_THEN_SEND的组合类型 | |

# 功能设计
## 配置
目前主要有两类配置
1. 指定集合操作算法的配置项，该config table定义在ucg_context.c中。
2. 指定Plan算法细节的配置项，该config table定义在Plan的源文件中。

为了支持User能通过ucg_context_init()时指定特定的配置项，需要提供
1. ucg_config_read()获取配置项指针`ucg_config_t*`
2. ucg_config_modify()修改特定的配置项

因为两类配置项是隔离的，所以无法将配置项字段定义在一个结构体中，只能分开获取，然后放在一个指针数组中
```
struct ucg_config {
    ucg_context xxxx
    ucs_ptr_array_t config_bundles[UCG_PLAN_TYPE_MAX];
}
```
因为不同Plan的配置项也是隔离的，相互并不感知，所以可能出现相同table name和table prefix，甚至一样的配置项名，需要增加检测机制。

在ucg_config_modify()时，只有配置项名，但并不知道该配置项名属于哪个table，因此只能一个个尝试，直到设置成功。为了便利，可以约定每个table的prefix必须不一样，这样可以通过比较配置项名是否有该prefix，从而找到对应的table。

还有一个问题是如何在初始化plan时，使用正确的modify后的config？config_bundles中Plan config的顺序与Plan模板数组的顺序一样，因此可以快速找到。

## Plan
分为两类
- Plan Template：Plan模板，包含Plan的元数据信息和函数，会注册到上层，供上层使用
- Plan Object：基于Plan模板实例化的Plan对象，包含Plan配置信息、集合操作参数（成员个数、数据）等

Plan Template代表一类算法，算法可根据配置信息和集合操作参数实例化一个Plan。

Plan Template注册到Plan模板库中，供Group使用。


## 指定Plan
通过数字指定特定集合操作所使用的Plan，比如`UCX_ALLREDUCE_PLAN=0`

## 选择Plan
1. Plan Pool通过Plan的`is_available()`函数获取支持集合操作的Plan。`is_available()`不止判断集合操作的参数、成员个数等，还需要判断依赖的环境是否就绪，比如Topology不可用时，topo-aware的Plan不能使用。这由Plan开发者保证。
2. Plan Pool通过Plan的`query()`函数获取信息，根据选择策略计算分数，从多个可用的Plan里选择最佳Plan。

`query()`提供的信息要根据选择策略来定，**后续完善**。

### 复用Plan
> 更新认知：一个实例化的Plan是包含集合操作所有信息的，如通讯对象、通讯数据等。

真正的写时复制实现起来比较麻烦，因此提前将Plan分为constant部分和mutable部分。对于constant部分通过指针引用，对于mutable部分通过constant部分里的指定函数初始化。Action同理。

### 配置Plan
Plan可以有自己的配置项，Plan对外提供读取、修改、释放配置的接口.
每个Plan管理自己的config table。init时读取配置信息，clone时直接引用配置信息
接口上允许外部指定config

ucg config的入口为ucg_context_init()，传入的config即为ucg配置项也含有plan配置项
struct ucg_config
{
    ucs_ptr_array_t config_bundles;
};

实际结构为ucg_config_bundle_t数组，顺序为ucg context配置项，然后是plan模板注册顺序

modify config时，只能在每个config bundle里尝试，可以通过约定table name作为table prefix，比较modify name和table prefix，如果一样再调用modify接口。

从ucs config接口上看，似乎并不会检查不同table之间是否存在相同的环境变量。

## 新增Plan
当实现一个新Plan时，需要在对应的枚举体中增加id

## Action执行框架
对于Reduce Action，通常是先Recv再Reduce的，此时Recv Action是无需做真正的内存拷贝的，可以直接使用UCT层的接收buffer进行Reduce Action。还有先Recv再Send的情况即单纯转发，此时可以直接将UCT buffer的内容发送出去。
```
do_action(action, data)
{
    ...
}

struct action_buf {
    uint8_t **buffers;
};
1. 先Recv再Reduce
create actions:
    recv action:
        recv.buffers[0] = UCG_BUFFER_HOLE; 
    reduce action:
        reduce.buffers[0] = UCG_BUFFER_REPLACE; // 第一个操作数
        reduce.buffers[1] = target; // 第二个操作数

2. 先Recv再Send
create actions:
    recv action:
        recv.buffers[0] = recv_buffer
    send action:
        reduce.buffers[0] = UCG_BUFFER_REPLACE

when recv data:
    do_action(action, data):
        switch(action type)
            SEND:
                 f send.buffers[0] == UCG_BUFFER_REPLACE
                    send.buffers[0] = data
                ucg_channel_send(send.buffer[0])
                break
            RECV:
                if recv.buffers[0] != UCG_BUFFER_HOLE
                    copy data to recv buffer
                break
            REDUCE:
                if reduce.buffers[0] == UCG_BUFFER_REPLACE
                    reduce.buffers[0] = data
                reduce(reduce.buffers[0], reduce.buffers[1])
            GENETIC:
                ucg_plan_action_generic(action, data)
                break
        if action->next != NULL
            do_action(action->next, data)
```

## datatype & op
datatype和op分为预定义和用户自定义两类，其中内部预定义可以细分为多种类型。
提供统一的接口，内部隐藏预定义类型和用户自定义类型的处理，对于用户自定义的类型调用用户注册的RTE函数。

不论datatype是何种类型，让Plan实现者总是基于内存是连续的前提进行开发，减少考虑的细节。
如果datatype是非连续的，Plan如何来决定Action Buffer？
1. 算法以datatype为单位进行Action Buffer的划分。
```
// 算法只需计算offset和count即可
action.type = RECV
action.buffers[0] = user_buffer + datatype->size * offset
action.lengths[0] = datatype->size * count

when recv data：
    length = 
    unpack(data, action.buffers[0], action.lengths[0])
```
3. 算法不以datatype为单位，而会在一个datatype中任意位置切割操作

Plan需要基于datatype+count决定Action buffer，
从底向上
1. Channel收发总是连续的
2. 实际数据位置可能是非连续的，需要通过pack()将非连续的数据放到连续的内存中
3. Plan实现Action时，总是认为数据位置是连续的



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


