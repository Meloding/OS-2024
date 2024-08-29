# OSLab

实验讲义与注意事项：左侧栏选择[Wiki](https://git.nju.edu.cn/WhereIsTheCatsTail/oslab2024spring/-/wikis/home)

提问：在群里或左侧栏选择[议题](https://git.nju.edu.cn/WhereIsTheCatsTail/oslab2024spring/-/issues)


### SUMMARY:

#### WEEK1 os-start

```
                +++++++++++++++++++++++++++++++++++++
USER SPACE      |            USER PROCESS           |<-------| 2. kernel load use process
                +++++++++++++++++++++++++++++++++++++       ||
=========================================================   ||
                +++++++++++++++++++++++++++++++++++++--------|
KERNEL SPACE    |              OS KERNEL            |
                +++++++++++++++++++++++++++++++++++++<--------| 1. load kernel from disk
                                                             ||
                                                            ++++++++++
                                                            |  DISK  |
                                                            ++++++++++
```


介绍计算器的启动（BIOS，MBR）以及OS的启动（程序的加载）
加载kernel进程， 并启动一个用户进程
1. boot/start.S
2. boot/boot.c
3. kernel/src/loader.c
4. kernel/src/proc.c: init_proc, proc_alloc, proc_run 

#### WEEK2 interrupt

```
                +++++++++++++++++++++++++++++++++++++
USER SPACE      |            USER PROCESS           |<------>| 
                +++++++++++++++++++++++++++++++++++++       ||  1. soft interrupt
=========================================================   ||
                +++++++++++++++++++++++++++++++++++++<------>|
KERNEL SPACE    |              OS KERNEL            |
                +++++++++++++++++++++++++++++++++++++
                                    |
                                    | 2. outer interrupt: devices
                    ++++++++++++<------->++++++++++
                    |  serial  |         |  time  |
                    ++++++++++++         ++++++++++    
```

