核心问题
1. 整个设计中是否已经没有可以减少的部分？
2. 抽象之间的交互是否考虑全面，并保留一定的扩展性？

## Planner是否有存在的必要？—— 否
不同Planner包含着不同的Plan，一旦划分Planner，那么一个Planner中的Plan依赖于另一个Planner里的Plan是相当违反直觉的，但实际中一个新算法会依赖原先的一些基础算法，而基础算法通常都位于builtin Planner。因此就有了这个问题。

从用户角度看
1. 因为用户的诉求是在所有可用的Plan中选择最佳的Plan，所以让用户指定Planner是没有意义的，用户必然指定ALL。

从实现角度看
1. 原先Planner还包含了收发流程，如果不同Planner包含不同的收发流程，那么确实有必要存在Planner。但现在会将收发流程提炼出来，因此Planner将只包含Plan，没有必要存在不同的Planner。
2. Plan之间本身就是隔离的，用不同的Planner包含不同的Plan，没什么意义。
3. 目前Planner的注册方式是通过UCS FRAMEWORK LOAD，支持动态加载，因此可以将闭源的Plan包含在一个Planner中。如果只是为了区别开闭源，那么可以通过编程工程来实现，即将开源的plan编译到libucg.so中，闭源的plan编译成一个库libucg_ext.so, 并在代码中默认加载libucg_ext.so。


