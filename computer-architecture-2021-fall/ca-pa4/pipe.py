#----------------------------------------------------------------
#
#  4190.308 Computer Architecture (Fall 2021)
#
#  Project #4: A 6-stage Pipelined RISC-V Simulator
#
#  November 25, 2021
#
#  Jin-Soo Kim (jinsoo.kim@snu.ac.kr)
#  Systems Software & Architecture Laboratory
#  Dept. of Computer Science and Engineering
#  Seoul National University
#
#----------------------------------------------------------------

from consts import *
from isa import *
from program import *
from components import *
from stages import *


#--------------------------------------------------------------------------
#   Pipeline implementation-specific constants
#--------------------------------------------------------------------------

S_IF      = 0
S_ID      = 1
S_EX      = 2
S_M1      = 3
S_M2      = 4
S_WB      = 5

S = [ 'IF', 'ID', 'EX', 'M1', 'M2', 'WB' ]


#--------------------------------------------------------------------------
#   Pipe: manages overall execution with logging support
#--------------------------------------------------------------------------

class Pipe(object):

    def __init__(self):
        self.name = self.__class__.__name__

    @staticmethod
    def set_stages(cpu, stages):
        Pipe.cpu = cpu
        Pipe.stages = stages
        Pipe.IF = stages[S_IF]
        Pipe.ID = stages[S_ID]
        Pipe.EX = stages[S_EX]
        Pipe.M1 = stages[S_M1]
        Pipe.M2 = stages[S_M2]
        Pipe.WB = stages[S_WB]

    @staticmethod
    def run(entry_point):
        from stages import IF

        IF.reg_pc = entry_point
        while True:
            # Run each stage 
            # Should be run in the reverse order because forwarding and 
            # hazard control logic depends on previous instructions
            Pipe.WB.compute()
            Pipe.M2.compute()
            Pipe.M1.compute()
            Pipe.EX.compute()
            Pipe.ID.compute()
            Pipe.IF.compute()

            # Update states
            Pipe.IF.update()
            Pipe.ID.update()
            Pipe.EX.update()
            Pipe.M1.update()
            Pipe.M2.update()
            ok = Pipe.WB.update()

            Stat.cycle      += 1
            if Pipe.WB.inst != BUBBLE:
                Stat.icount += 1
                opcode = RISCV.opcode(Pipe.WB.inst)
                if isa[opcode][IN_CLASS] == CL_ALU:
                    Stat.inst_alu += 1
                elif isa[opcode][IN_CLASS] == CL_MEM:
                    Stat.inst_mem += 1
                elif isa[opcode][IN_CLASS] == CL_CTRL:
                    Stat.inst_ctrl += 1

            # Show logs after executing a single instruction
            if Log.level >= 6:
                Pipe.cpu.rf.dump()                      # dump register file
            if Log.level >= 7:
                Pipe.cpu.dmem.dump(skipzero = True)     # dump dmem
            if Log.level >= 4:
                print('-'*50)

            if not ok:
                break;

        # Handle exceptions, if any
        if (Pipe.WB.exception & EXC_DMEM_ERROR):
            print("Exception '%s' occurred at 0x%08x -- Program terminated" % (EXC_MSG[EXC_DMEM_ERROR], Pipe.WB.pc))
        elif (Pipe.WB.exception & EXC_EBREAK):
            print("Execution completed")
        elif (Pipe.WB.exception & EXC_ILLEGAL_INST):
            print("Exception '%s' occurred at 0x%08x -- Program terminated" % (EXC_MSG[EXC_ILLEGAL_INST], Pipe.WB.pc))
        elif (Pipe.WB.exception & EXC_IMEM_ERROR):
            print("Exception '%s' occurred at 0x%08x -- Program terminated" % (EXC_MSG[EXC_IMEM_ERROR], Pipe.WB.pc))

        if Log.level > 0:
            if Log.level < 6:
                Pipe.cpu.rf.dump()                      # dump register file
            if Log.level > 1 and Log.level < 7:
                Pipe.cpu.dmem.dump(skipzero = True)     # dump dmem
       
    # This function is called by each stage after updating its states
    @staticmethod
    def log(stage, pc, inst, info):

        if Stat.cycle < Log.start_cycle:
            return
        if Log.level < 5:
            info = ''
        if Log.level >= 4 or (Log.level == 3 and stage == S_WB):
            print("%d [%s] 0x%08x: %-30s%-s" % (Stat.cycle, S[stage], pc, Program.disasm(pc, inst), info))
        else:
            return

