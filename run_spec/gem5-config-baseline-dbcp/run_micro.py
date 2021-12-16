# Derived from gem5-art

from __future__ import print_function

import argparse
import m5
from m5.objects import TimingSimpleCPU, DerivO3CPU, MinorCPU
from m5.objects import LTAGE, SimpleMemory
from m5.objects import Root
from m5.objects import *
import time
from system import BaseTestSystem
from system import InfMemory, SingleCycleMemory, SlowMemory

BranchPredictor = LTAGE

class IntALU(FUDesc):
    opList = [ OpDesc(opClass='IntAlu',opLat=1) ]
    count = 32

class IntMultDiv(FUDesc):
    opList = [ OpDesc(opClass='IntMult', opLat=1),
               OpDesc(opClass='IntDiv', opLat=20, pipelined=False) ]

    # DIV and IDIV instructions in x86 are implemented using a loop which
    # issues division microops.  The latency of these microops should really be
    # one (or a small number) cycle each since each of these computes one bit
    # of the quotient.
    if buildEnv['TARGET_ISA'] in ('x86'):
        opList[1].opLat=1

    count=32

class FP_ALU(FUDesc):
    opList = [ OpDesc(opClass='FloatAdd', opLat=1),
               OpDesc(opClass='FloatCmp', opLat=1),
               OpDesc(opClass='FloatCvt', opLat=1) ]
    count = 32

class FP_MultDiv(FUDesc):
    opList = [ OpDesc(opClass='FloatMult', opLat=1),
               OpDesc(opClass='FloatMultAcc', opLat=1),
               OpDesc(opClass='FloatMisc', opLat=1),
               OpDesc(opClass='FloatDiv', opLat=1, pipelined=False),
               OpDesc(opClass='FloatSqrt', opLat=1, pipelined=False) ]
    count = 32

class SIMD_Unit(FUDesc):
    opList = [ OpDesc(opClass='SimdAdd', opLat=1),
               OpDesc(opClass='SimdAddAcc', opLat=1),
               OpDesc(opClass='SimdAlu', opLat=1),
               OpDesc(opClass='SimdCmp', opLat=1),
               OpDesc(opClass='SimdCvt', opLat=1),
               OpDesc(opClass='SimdMisc', opLat=1),
               OpDesc(opClass='SimdMult', opLat=1),
               OpDesc(opClass='SimdMultAcc', opLat=1),
               OpDesc(opClass='SimdShift', opLat=1),
               OpDesc(opClass='SimdShiftAcc', opLat=1),
               OpDesc(opClass='SimdSqrt', opLat=1),
               OpDesc(opClass='SimdFloatAdd', opLat=1),
               OpDesc(opClass='SimdFloatAlu', opLat=1),
               OpDesc(opClass='SimdFloatCmp', opLat=1),
               OpDesc(opClass='SimdFloatCvt', opLat=1),
               OpDesc(opClass='SimdFloatDiv', opLat=1),
               OpDesc(opClass='SimdFloatMisc', opLat=1),
               OpDesc(opClass='SimdFloatMult', opLat=1),
               OpDesc(opClass='SimdFloatMultAcc', opLat=1),
               OpDesc(opClass='SimdFloatSqrt', opLat=1) ]
    count = 32

class ReadPort(FUDesc):
    opList = [ OpDesc(opClass='MemRead'),
               OpDesc(opClass='FloatMemRead') ]
    count = 32

class WritePort(FUDesc):
    opList = [ OpDesc(opClass='MemWrite'),
               OpDesc(opClass='FloatMemWrite') ]
    count = 32

class RdWrPort(FUDesc):
    opList = [ OpDesc(opClass='MemRead'), OpDesc(opClass='MemWrite'),
               OpDesc(opClass='FloatMemRead'), OpDesc(opClass='FloatMemWrite')]
    count = 32

class IprPort(FUDesc):
    opList = [ OpDesc(opClass='IprAccess', opLat = 1, pipelined = False) ]
    count = 32

class Ideal_FUPool(FUPool):
    FUList = [ IntALU(), IntMultDiv(), FP_ALU(), FP_MultDiv(), ReadPort(),
               SIMD_Unit(), WritePort(), RdWrPort(), IprPort() ]


class Minor4CPU(MinorCPU):
    branchPred = BranchPredictor()
    fuPool = Ideal_FUPool()
    decodeInputWidth  = 4
    executeInputWidth = 4
    executeIssueLimit = 4
    executeCommitLimit = 4


class O3_W256CPU(DerivO3CPU):
    branchPred = BranchPredictor()
    fuPool = Ideal_FUPool()
    fetchWidth = 32
    decodeWidth = 32
    renameWidth = 32
    dispatchWidth = 32
    issueWidth = 32
    wbWidth = 32
    commitWidth = 32
    squashWidth = 32
    fetchQueueSize = 256
    LQEntries = 250
    SQEntries = 250
    numPhysIntRegs = 256
    numPhysFloatRegs = 256
    numIQEntries = 256
    numROBEntries = 256


class O3_W2KCPU(DerivO3CPU):
    branchPred = BranchPredictor()
    fuPool = Ideal_FUPool()
    fetchWidth = 32
    decodeWidth = 32
    renameWidth = 32
    dispatchWidth = 32
    issueWidth = 32
    wbWidth = 32
    commitWidth = 32
    squashWidth = 32
    fetchQueueSize = 256
    LQEntries = 250
    SQEntries = 250
    numPhysIntRegs = 1024
    numPhysFloatRegs = 1024
    numIQEntries = 1024
    numROBEntries = 1024

class SimpleCPU(TimingSimpleCPU):
    branchPred = BranchPredictor()

class DefaultO3CPU(DerivO3CPU):
    branchPred = BranchPredictor()

# Add more CPUs under test before this
valid_cpus = [SimpleCPU, Minor4CPU, DefaultO3CPU, O3_W256CPU, O3_W2KCPU]
valid_cpus = {cls.__name__[:-3]:cls for cls in valid_cpus}

# Add more Memories under test before this
valid_memories = [InfMemory, SingleCycleMemory, SlowMemory]
valid_memories = {cls.__name__[:-6]:cls for cls in valid_memories}

# Add more Memory Controllers under test before this
valid_mem_ctls = [DDR3_1600_8x8, DDR3_2133_8x8, LPDDR2_S4_1066_1x32, HBM_1000_4H_1x64]
valid_mem_ctls = {cls.__name__:cls for cls in valid_mem_ctls}

parser = argparse.ArgumentParser()
parser.add_argument('cpu', choices = valid_cpus.keys())
parser.add_argument('memory_ctl', choices = valid_mem_ctls.keys())
parser.add_argument('binary', type = str, help = "Path to binary to run")
parser.add_argument('arg', type = str, help = "Argument of the binary")
parser.add_argument("--clock", action="store",
                      default='1GHz',
                      help = """Top-level clock for blocks running at system
                      speed""")
parser.add_argument("-W", action="store",
                      default='3000000',
                      help = """Warmup Insructions""")
parser.add_argument("-I", action="store",
                      default='3000000',
                      help = """How many instructions actually run""")
args  = parser.parse_args()

class MySystem(BaseTestSystem):
    _CPUModel = valid_cpus[args.cpu]
    _Clk         = args.clock
    #_MemoryModel = MemCtrl()
    _MemoryModel = SlowMemory
    

print (args.clock)
warmup_inst = args.W
run_inst = args.I
print ("Warm up instruction: "+str(warmup_inst))
print ("Execution instruction: "+str(run_inst))
system = MySystem()
if args.arg != '':
    system.setTestBinaryWithArgs(args.binary, args.arg.split(' '))
else:
    system.setTestBinary(args.binary)

system.setMemCtrl(valid_mem_ctls[args.memory_ctl])
system.cpu.max_insts_any_thread = warmup_inst if int(warmup_inst) > 0 else run_inst
root = Root(full_system = False, system = system)
m5.instantiate()

start_tick = m5.curTick()
start_insts = system.totalInsts()
globalStart = time.time()
exit_event = m5.simulate()
m5.checkpoint('afterwarmup')

print("Exit Event" + exit_event.getCause())
if exit_event.getCause() == "workbegin" or (exit_event.getCause() == "a thread reached the max instruction count" and int(warmup_inst) > 0):
    # Reached the start of ROI
    # start of ROI is marked by an
    # m5_work_begin() call
    m5.stats.reset()
    start_tick = m5.curTick()
    start_insts = system.totalInsts()
    print("Resetting stats at the start of ROI!")
    system.cpu.max_insts_any_thread = run_inst
    m5.instantiate('afterwarmup')
    exit_event = m5.simulate()
    
# Reached the end of ROI
# Finish executing the benchmark with kvm cpu
if exit_event.getCause() == "workend":
    # Reached the end of ROI
    # end of ROI is marked by an
    # m5_work_end() call
    print("Dump stats at the end of the ROI!")
    m5.stats.dump()
    end_tick = m5.curTick()
    end_insts = system.totalInsts()
    m5.stats.reset()
else:
    print("Terminated simulation before reaching ROI!")
    print("Exit Event" + exit_event.getCause())
    m5.stats.dump()
    end_tick = m5.curTick()
    end_insts = system.totalInsts()
    print("Performance statistics:")
    print("Simulated time: %.2fs" % ((end_tick-start_tick)/1e12))
    print("Instructions executed: %d" % ((end_insts-start_insts)))
    print("Ran a total of", m5.curTick()/1e12, "simulated seconds")
    print("Total wallclock time: %.2fs, %.2f min" % \
          (time.time()-globalStart, (time.time()-globalStart)/60))
    exit()
    

