# Topology
进程的拓扑信息，包含每个进程所在的节点、NUMA、Socket等信息，依据这些信息可以得出两两进程间的距离信息。部分算法需要距离信息来决定集合操作的通讯步骤。

距离信息是一个二维矩阵，假设通信组内有4个进程：P0、P1、P2、P3，其距离信息如下
```
   P0 P1 P2 P3
P0 0  1  2  3
P1 1  0  4  5
P2 2  4  0  6
P3 3  5  6  0
```
如果存储二维矩阵，那么空间复杂度为O(N^2)，N为进程个数。当进程数增加至1万，那么每个进程都需要占用100MB，假设PPN=100，那么单节点就由10GB内存被占用。

## 设计
1. 使用共享内存，每个节点保存一份即可。
2. 因为`distance[x][y]`和`distance[y][x]`是一样的，所以只需保存上三角矩阵
3. 因为`distance[x][x]`必然是0，所以无需保存对角线

综上可以使用一维数组来保存距离信息,
1. 数组[0, N-1)保存0到1~N-1的距离
2. [N-1, 2N-3)保存1到2~N-1的距离
3. 以此类推

伪码如下
```
if x == y
    return 0
if x > y
    swap(x, y)
the distance of x and y :=  distance[x*(N-1+N-x)/2 + y-x-1]
```

## 共享内存
使用shmget、shmat需要传递semid，而使用shm_open、mmap创建共享内存只需要提前规定文件名，因此使用shm_open、mmap。需要使用者传递节点内唯一的ID，保证shm_open的文件名唯一。
1. 所有进程执行`sem_open(name, O_CREATE | O_EXCL, 066, 0)`，只有一个进程能创建成功，由该进程负责topology的初始化。
2. 其余进程重新执行`sem_open(name, 0)`，并执行`sem_wait()`等待topology初始化完成。

**前提**：所有进程都会调用`ucg_topo_init()`
1. openmpi：MPI_Init()中会以`MPI_COMM_WORLD`使能ucg module，使能时会调用`ucg_group_create()`，因此将`ucg_topo_init()`放在`ucg_group_create()`中调用。
2. 若迁移到其他运行环境，就要求先创建包含所有进程的group。
> ucg component初始化时，`MPI_COMM_WORLD`还未初始化，因此无法放在`ucg_rte_init()`或`ucg_context_init()`中。
