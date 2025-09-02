# 4190.308 Computer Architecture (Fall 2021)
# Project #4: A 6-stage Pipelined RISC-V Simulator
### Due: 11:59PM, December 18 (Saturday)

## Introduction

The goal of this project is to understand how a pipelined processor works. You need to build a 6-stage pipelined RISC-V simulator called `snurisc6` in Python that supports the most of RV32I base instruction set.

## Processor microarchitecture

The traditional 5-stage pipelined processor consists of IF, ID, EX, MM, and WB stages. However, we have decided to replace our data memory with a new memory device. The new memory device is a lot cheaper than the previous one, but it takes 2 cycles to access. In order to access the new data memory in one cycle, the clock period should be doubled, which will penalize all the instructions. Instead, we can break the traditional MM stage into two, namely M1 and M2. The `lw` and `sw` instructions will start the memory access at the beginning of the M1 stage and those memory operations are completed at the end of the M2 stage. In this way, we can still make use of the new memory device without increasing the clock period. 

The target RISC-V processor `snurisc6` consists of six pipeline stages: IF, ID, EX, M1, M2, and WB. The following briefly summarizes the tasks performed in each stage:

* IF: Fetches an instruction from imem (instruction memory)
* ID: Decodes the instruction and reads the register file 
* EX: Performs arithmetic/logical computation and determines the branch outcome
* M1: Starts accessing dmem (data memory), if necessary
* M2: Completes accessing dmem (data memory), if necessary
* WB: Writes back the result to the register file

The `snurisc6` processor has the following characteristics:

* The control logic is located in the ID stage as in `snurisc5`.
* If the pipeline needs to be stalled, it is detected in the ID stage. 
* The forwarding (or bypassing) detection and resolution is done at the end of the ID stage. Note that this is different from the pipeline design outlined in the textbook.
* The write to the register file is done at the end of the WB stage (not in the middle of the WB stage). Therefore, when an instruction tries to read a register in the ID stage while the preceding instruction is writing a value to the same register in the WB stage at the same cycle, the value should be forwarded as well. Please note that this behavior of the register file is different from what is assumed in the textbook.
* The outcome of the conditional branch is determined at the end of the EX stage (Again, this is different from the textbook design). Once the branch outcome is known, any mispredicted branch should be handled immediately.


## Problem specification

This project assignment consists of the following four parts.

### Part 1. Implementing a 6-stage pipelined RISC-V processor simulator (30 points)

Your first task is to implement `snurisc6`, a 6-stage pipelined RISC-V processor simulator in Python. The reference 5-stage pipelined RISC-V processor simulator is available in the `pipe5` directory of [the PyRISC Project](https://github.com/snu-csl/pyrisc). The requirements for implementing `snurisc6` are summarized below:

* `snurisc6` should accept the same RISC-V executable file accepted by the reference `snurisc5` simulator.

* `snurisc6` and `snurisc5` should produce the same results (in terms of register values and memory states).

* Data forwarding should be fully implemented wherever necessary.

* In case data forwarding cannot solve the dependency among instructions (e.g., load-use hazard), the pipeline should be stalled.

* When the M1 stage is occupied by `lw` or `sw` instruction, the following `lw` or `sw` instruction should be stalled for one cycle because M1 and M2 stages should be used by a single `lw` or `sw` instruction for two consecutive cycles. This is a form of structural hazard on data memory.

* You should minimize the number of stalled cycles. 

Example 1: load-use hazard
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    lw   t0, 0(s0)       IF  ID  EX  M1  M2  WB
    addi t0, t0, 1           IF  ID  ID  ID  EX  M1  M2  WB
    add  t2, t0, t2              IF  IF  IF  ID  EX  M1  M2  WB
```

Example 2: M1-M2 hazard
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    lw   t0, 0(s0)       IF  ID  EX  M1  M2  WB
    lw   t1, 4(s0)           IF  ID  ID  EX  M1  M2  WB
    add  t0, t0, t1              IF  IF  ID  ID  ID  EX  M1  M2  WB
```

Exmpale 3: when both load-use hazard and M1-M2 hazard occur
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    lw   t0, 0(s0)       IF  ID  EX  M1  M2  WB
    sw   t0, 4(s0)           IF  ID  ID  ID  EX  M1  M2  WB
```

### Part 2. Implementing the BTFNT branch prediction scheme (30 points)

By default, the "__always-not-taken__" branch prediction scheme is used in ``snurisc5``, i.e., the instruction next to the conditional branch instruction is fetched immediately at the next cycle assuming that the branch will not be taken. 

Instead, your second task is to implement the "__BTFNT (Backward branch as Taken, Forward branch as Not Taken)__" branch prediction scheme in `snurisc6`. In the __BTFNT__ branch prediction scheme, the instruction in the branch target is immediately fetched for backward branches, while the instruction next to the branch instruction is fetched for forward branches. Similar to the __always-not-taken__ prediction scheme, any instructions that are incorrectly fetched should be cancelled (by converting them into BUBBLEs) when the prediction was wrong. Here are more detailed descriptions for implementing the __BTFNT__ branch prediction scheme.

* The branch prediction should be performed in the IF stage in order to fetch the next instruction immediately. This means that we need to put an additional logic in the IF stage to identify whether the fetched instruction is one of the conditional branch instructions (i.e., `beq`, `bne`, `bge`, `bgeu`, `blt`, `bltu`).

* When the current instruction is a branch instruction, you need to extract the offset value (or _displacement_) from the instruction word to find out whether it is a forward branch (displacement >= 0) or a backward branch (displacement < 0). For backward branches (which are predicted to be taken), it is required to compute the branch target address in the IF stage. This can be done by adding the displacement to the current `pc`. You can use the adder named `Pipe.cpu.adder_if` for this purpose.

* The branch outcome is determined in the EX stage. If the prediction was right, there is nothing we need to do. However, if the prediction was wrong, we need to cancel the incorrectly fetched instructions. At the time we know the prediction was wrong in the EX stage, two instructions are already fetched and running in the IF and ID stages of the pipeline. Therefore, these two instructions should be converted to BUBBLEs.

* Finally, when the prediction was wrong, we need to forward the correct value for the next `pc`. 

* We can treat the `jal` instruction as backward branches, i.e., it is always predicted as taken. The target address of the ``jal`` instruction is given by the ``pc`` + 20-bit offset (21-bit after adjustment). So, we can use the same adder to compute the target address. For the ``jal`` instruction, the difference is that we are never wrong about the prediction.

* The handling of the `jalr` instruction is a little bit tricky, as its target address is given by the `rs1` + 12-bit offset. In order to compute the target address in the IF stage for the `jalr` instruction, we need another read port in the register file (due to structural hazard). Also, it requires another forwarding path from later stages to the IF stage and pipeline stalls for the possible dependency with the preceding instructions on the `rs1` register. To make the problem simpler, the `jalr` instruction is handled in the same way as in `snurisc5`; the instructions next to the `jalr` instruction are fetched until we have the target address in the EX stage and then the incorrectly fetched instructions are converted into BUBBLEs while the target address is forwarded to the next `pc` value immediately. The better solution for the `jalr` instruction is discussed in the part 3 of this project assignment.

Example 4: forward branch (correct prediction)
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    bne  t0, t0, L1      IF  ID  EX  M1  M2  WB
    add  t1, t2, t3          IF  ID  EX  M1  M2  WB
    addi t1, t1, -1              IF  ID  EX  M1  M2  WB
    sub  t4, t1, t2                  IF  ID  EX  M1  M2  WB
    ...
    ...
L1: sub  t5, t6, t3          
    xori t5, t5, 1               
    add  t6, t6, t5                  
    addi t6, t6, 10                      
```

Example 5: forward branch (wrong prediction)
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    beq  t0, t0, L1      IF  ID  EX  M1  M2  WB
    add  t1, t2, t3          IF  ID  -   -   -
    addi t1, t1, -1              IF  -   -   -
    ...
    ...
L1: sub  t5, t6, t3                  IF  ID  EX  M1  M2  WB
    xori t5, t5, 1                       IF  ID  EX  M1  M2  WB
    add  t6, t6, t5                          IF  ID  EX  M1  M2  WB
```

Example 6: backward branch (correct prediction)
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
L1: add  t1, t2, t3          IF  ID  EX  M1  M2  WB            
    addi t1, t1, -1              IF  ID  EX  M1  M2  WB
    sub  t4, t1, t2                  IF  ID  EX  M1  M2  WB
    ...
    ...          
    beq  t0, t0, L1      IF  ID  EX  M1  M2  WB
    sub  t5, t6, t3          
    xori t5, t5, 1                                            
```

Example 7: backward branch (wrong prediction)
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
L1: add  t1, t2, t3          IF  ID  -   -   -   -            
    addi t1, t1, -1              IF  -   -   -   -   -
    sub  t4, t1, t2                  
    ...
    ...          
    bne  t0, t0, L1      IF  ID  EX  M1  M2  WB
    sub  t5, t6, t3                  IF  ID  EX  M1  M2  WB
    xori t5, t5, 1                       IF  ID  EX  M1  M2  WB  
```

Example 8: `jal` instruction
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    jal  ra, L1          IF  ID  EX  M1  M2  WB
    add  t1, t2, t3
    addi t1, t1, -1
    ...
    ...
L1: sub  t5, t6, t3          IF  ID  EX  M1  M2  WB
    xori t5, t5, 1               IF  ID  EX  M1  M2  WB
    add  t6, t6, t5                  IF  ID  EX  M1  M2  WB
```

Example 9: `jalr` instruction (assuming `ra` contains the address of `L1`)
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    jalr x0, 0(ra)       IF  ID  EX  M1  M2  WB
    add  t1, t2, t3          IF  ID  -   -   -   -
    addi t1, t1, -1              IF  -   -   -   -   -            
    ...
    ...
L1: sub  t5, t6, t3                  IF  ID  EX  M1  M2  WB
    xori t5, t5, 1                       IF  ID  EX  M1  M2  WB
    add  t6, t6, t5                          IF  ID  EX  M1  M2  WB
```

### Part 3: Implementing return address stack (20 points)

As explained in the Part 2 of this document, it is difficult to obtain the target address of the `jalr` instruction in the IF stage. One way to solve this problem is to use the fact that the `jalr` instruction is mostly used for implementing  returns from function calls. This means that the target address for the `jalr` instruction was written into the `ra` (or `x1`) register by a previous `jal` or `jalr` instruction that invoked the function call. 

According to the RISC-V ISA manual, the `jalr` instruction with `rd = x0` and `rs1 = x1` is commonly used as the return instruction from a function call. Besides, the `jal` or `jalr` instruction with `rd = x1` is commonly used as the jump to invoke a function call. To predict the target address of the `jalr` instruction, we can introduce the __return address stack (RAS)__ to our `snurisc6` processor. The RAS is a small, fixed-size memory of 32-bit return addresses maintained with stack discipline. Every time a function call is invoked, its return address is pushed into the RAS. Every time the control is returned from a function call, an address is popped from the RAS and the CPU continues fetching from the address. The implementation details can be summarized as follows.

* First, you need to identify whether the fetched instruction is `jal` or `jalr` instruction in the IF stage. 

* If the current instruction is predicted to make a function call (i.e., `rd = x1` for `jal` and `jalr` instructions), push the return address (= `pc` + 4) into the RAS.

* If the current instruction is predicted to return from a function call (i.e., `rd = x0`, `rs1 = x1`, and `offset = 0` for `jalr` instruction), pop an address from the RAS and use that address for the next `pc` value. 

* Note that the RAS provides a _predicted return address_ for the `jalr` instruction and the actual return address can be different from the one popped from the RAS. In this case, incorrectly fetched instructions should be handled in the same way as the mispredicted branches. 

* The `class RAS` in the `components.py` file models the required hardware component for the RAS. A RAS object with 8-entries (named `Pipe.cpu.ras`) is already available when the CPU is initialized. You can use this object to push a value to the RAS (`Pipe.cpu.ras.push()`) or pop a value from the RAS (`Pipe.cpu.ras.pop()`).

* If you try to pop an address from an empty RAS, it will return the address `0x00000000` with the status value of `False`. In this case, just fetch the instructions next to the `jalr` instruction.

* Note that the RAS can be corrupted when there are `jal` and `jalr` instructions in the mispredicted control path. Preventing the RAS corruption in this case is an advanced topic, so you just leave it corrupted in this project assignment. 


Example 10: `jalr` instruction with RAS (RAS provides the correct address)
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
L0: jal  ra, L1          IF  ID  EX  M1  M2  WB                    // L0+4 pushed to RAS
    add  t1, t2, t3                  IF  ID  EX  M1  M2  WB 
    addi t1, t1, -1                      IF  ID  EX  M1  M2  WB            
    ...
    ...
L1: sub  a0, a0, a1          IF  ID  EX  M1  M2  WB
    jalr x0, 0(ra)               IF  ID  EX  M1  M2  WB            // L0+4 popped from RAS
```


Example 11: `jalr` instruction with RAS (RAS has the address of `L0`, but `ra` contains the address of `L1`)
```
                         C0  C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 (cycle)
    jalr x0, 0(ra)       IF  ID  EX  M1  M2  WB                    // L0 popped from RAS in C0
    add  t1, t2, t3                       
    ...
    ...
L0: sub  a0, a0, a1          IF  ID  -   -   -   -
    addi t1, t1, -1              IF  -   -   -   -   -      
    ...
    ...
L1: sub  t5, t6, t3                  IF  ID  EX  M1  M2  WB        //  On C2, got the actual address L1
    xori t5, t5, 1                       IF  ID  EX  M1  M2  WB
    add  t6, t6, t5                          IF  ID  EX  M1  M2  WB
```


### Part 4: Design document (20 points)

You need to prepare and submit the design document (in PDF file) for the `snurisc6` processor. If you design the 6-stage RISC-V pipeline correctly with satisfying all the above requirements, you will get 20 points even if your implementation does not work. Your design document should answer the following questions:

1. What does the overall pipeline architecture look like? (5 points)

 * We provide you with the [snurisc6-design.pdf](https://github.com/snu-csl/ca-pa4/blob/master/snurisc6-design.pdf) file that has an empty diagram of pipeline stages and hardware components. You need to complete this diagram according to your pipeline design. A hand-drawn diagram is OK. You don't have to spend a lot of time to make it fancy. Please take a picture of your diagram and attach it in your design document.

2. About Part 1: When do structural/data hazards occur and how do you deal with them? (5 points)

 * Specify all the possible conditions (using control signals) when structural/data hazards can occur
 * Show the required control logic to deal with structural/data hazards
 
3. About Part 2: When do control hazards occur and how do you deal with them with the __BTFNT__ branch prediction scheme? (5 points)

 * Again, specify all the possible cases (using control signals) when control hazards can occur 
 * Show the required control logic to deal with control hazards

4. About Part 3: How do you use the RAS? (5 points)
 * Show the required control logic to use the RAS


### Getting started

We provide you with the skeleton code that can be downloaded from https://github.com/snu-csl/ca-pa4. To download the skeleton code, please take the following step:

```
$ git clone https://github.com/snu-csl/ca-pa4.git
```

Note that the `snurisc6` simulator is based on the reference 5-stage pipelined simulator (`snurisc5`) available in [the PyRISC project](https://github.com/snu-csl/pyrisc). We have slightly changed the simulator structure so that you only need to modify the ``stages.py`` file. Currently, `snurisc6` just supports `lw` and `sw` instructions without implementing any hazard detection and control logic. Please refer to the [snurisc6-skel.pdf](https://github.com/snu-csl/ca-pa4/blob/master/snurisc6-skel.pdf) file for the current pipeline structure of the `snurisc6` simulator.

Your task is to make it work correctly for any combination of instructions. You may find the [GUIDE.md](https://github.com/snu-csl/pyrisc/blob/master/pipe5/GUIDE.md) file in the PyRISC project useful, which describes the overall architecture and implementation details of the `snurisc5` simulator.

In the PyRISC project, several RISC-V executable files are available such as `fib`, `sum100`, `forward`, `branch`, and `loaduse`. You can test your simulator with these programs. Also, it is highly recommended to write your own test programs to see how your simulator works in a particular situation. Note that, for the given RISC-V executable file, `snurisc` (ISA simulator), `snurisc5` (5-stage implementation), and your `snurisc6` (6-stage implementation) all should produce the same results in terms of register values and memory states. The only difference will be the number of cycles you need to execute the program.

The following example shows how you can run the executable file `forward` on the `snurisc6` simulator (We assume that `pyrisc` is also downloaded in the same directory as `ca-pa4`).

```
$ cd ca-pa4
$ ./snurisc6.py -l 4 ../pyrisc/asm/forward   
Loading file ../pyrisc/asm/forward
0 [IF] 0x80000000: addi   t6, zero, 0
0 [ID] 0x00000000: BUBBLE
0 [EX] 0x00000000: BUBBLE
0 [M1] 0x00000000: BUBBLE
0 [M2] 0x00000000: BUBBLE
0 [WB] 0x00000000: BUBBLE
--------------------------------------------------
1 [IF] 0x80000004: addi   t0, zero, 1
1 [ID] 0x80000000: addi   t6, zero, 0
1 [EX] 0x00000000: BUBBLE
1 [M1] 0x00000000: BUBBLE
1 [M2] 0x00000000: BUBBLE
1 [WB] 0x00000000: BUBBLE
--------------------------------------------------
...

(cycles omitted)

...
--------------------------------------------------
13 [IF] 0x80000034: (illegal)
13 [ID] 0x80000030: BUBBLE
13 [EX] 0x8000002c: BUBBLE
13 [M1] 0x80000028: BUBBLE
13 [M2] 0x80000024: BUBBLE
13 [WB] 0x80000020: ebreak
--------------------------------------------------
Execution completed
Registers
=========
zero ($0): 0x00000000    ra ($1):   0x00000000    sp ($2):   0x00000000    gp ($3):   0x00000000
tp ($4):   0x00000000    t0 ($5):   0x00000001    t1 ($6):   0x00000002    t2 ($7):   0x00000003
s0 ($8):   0x00000000    s1 ($9):   0x00000000    a0 ($10):  0x00000000    a1 ($11):  0x00000000
a2 ($12):  0x00000000    a3 ($13):  0x00000000    a4 ($14):  0x00000000    a5 ($15):  0x00000000
a6 ($16):  0x00000000    a7 ($17):  0x00000000    s2 ($18):  0x00000000    s3 ($19):  0x00000000
s4 ($20):  0x00000000    s5 ($21):  0x00000000    s6 ($22):  0x00000000    s7 ($23):  0x00000000
s8 ($24):  0x00000000    s9 ($25):  0x00000000    s10 ($26): 0x00000000    s11 ($27): 0x00000000
t3 ($28):  0x00000000    t4 ($29):  0x00000000    t5 ($30):  0x00000000    t6 ($31):  0x00000000
Memory 0x80010000 - 0x8001ffff
==============================
9 instructions executed in 14 cycles. CPI = 1.556
Data transfer:    0 instructions (0.00%)
ALU operation:    8 instructions (88.89%)
Control transfer: 1 instructions (11.11%)
```

## Restrictions

* You should not change any files other than `stages.py`. 

* Your `stages.py` file should not contain any `print()` function even in comment lines. Please remove them before you submit your code to the server.

* Your simulator should minimize the number of stalled cycles.

* Your code should finish within a reasonable number of cycles. If your simulator runs beyond the predefined threshold, you will get the `TIMEOUT` error.

* __The number of submissions to the `sys` server will be limited to 50 times__.


## Hand in instructions

* Submit only the `stages.py` file to the submission server.

* Also, submit the design document (in PDF file only) to the submission server.

* The `sys` server will be closed at 11:59PM on December 22nd. This is the firm deadline.


## Logistics

* You will work on this project alone.

* Only the upload submitted before the deadline will receive the full credit. 25% of the credit will be deducted for every single day delay.

* You can use up to 4 slip days during this semester. If your submission is delayed by 1 day and if you decided to use 1 slip day, there will be no penalty. In this case, you should explicitly declare the number of slip days you want to use in the QnA board of the submission server after each submission.

* Any attempt to copy others' work will result in heavy penalty (for both the copier and the originator). Don't take a risk.


This is the final project. I hope you enjoyed!



---
[Jin-Soo Kim](mailto:jinsoo.kim_AT_snu.ac.kr)  
[Systems Software and Architecture Laboratory](http://csl.snu.ac.kr)  
[Dept. of Computer Science and Engineering](http://cse.snu.ac.kr)  
[Seoul National University](http://www.snu.ac.kr)
