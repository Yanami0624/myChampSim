1. 新增is_malloc字段
tracer_instruction.h
    struct input_instr ADD 
        is_malloc

2. 增加插桩函数
champsim_tracer.cpp
    ADD 
        MallocEntry(), MallocExit(), FreeEntr ImageLoad()

3. 为ooo_model_instr也增加is_malloc字段
instruction.h
    struct ooo_model_instr CHANGE 
        construction()
ooo_cpu.cc
    CHANGE  
        O3_CPU::initialize_instruction()

4. 地址-对象表
ooo_cpu.h
    ADD
        struct ObjectInfo
ooo_cpu.cc
    ADD
        handle_malloc_event
        handle_free_event

5. cache层统计object的 access, hit, miss
cache.cc

6. 输出
plain_printer.cc
