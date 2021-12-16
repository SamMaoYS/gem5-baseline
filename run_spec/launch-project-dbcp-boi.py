from run import gem5Run
import os
import sys
from uuid import UUID
from itertools import starmap
from itertools import product
import multiprocessing as mp
import argparse

def worker(run):
    run.run()
    json = run.dumpsJson()
    print(json)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('N', action="store",
                      default=1, type=int,
                      help = """Number of cores used for simulation""")
    args  = parser.parse_args()

    #cpu_types = ['Simple','Minor4','DefaultO3']
    cpu_types = ['Simple','DefaultO3']
    #mem_ctls = ['DDR3_1600_8x8', 'DDR3_2133_8x8', 'LPDDR2_S4_1066_1x32', 'HBM_1000_4H_1x64']
    mem_ctls = ['DDR3_1600_8x8']

    bm_list = ['510.parest_r','541.leela_r','641.leela_s','531.deepsjeng_r','631.deepsjeng_s','505.mcf_r','605.mcf_s','523.xalancbmk_r', '623.xalancbmk_s']
    parameters_list = [
        '/data/benchmarks/spec2017/benchspec/CPU/510.parest_r/data/test/input/test.prm', #510
        '/data/benchmarks/spec2017/benchspec/CPU/541.leela_r/data/test/input/test.sgf',
        '/data/benchmarks/spec2017/benchspec/CPU/541.leela_r/data/test/input/test.sgf', #541&641
        '',
        '', #531&631
        '/data/benchmarks/spec2017/benchspec/CPU/505.mcf_r/data/test/input/inp.in',
        '/data/benchmarks/spec2017/benchspec/CPU/505.mcf_r/data/test/input/inp.in',#505&605
        '/data/benchmarks/spec2017/benchspec/CPU/523.xalancbmk_r/data/test/input/test.xml /data/benchmarks/spec2017/benchspec/CPU/523.xalancbmk_r/data/refrate/input/xalanc.xsl',
        '/data/benchmarks/spec2017/benchspec/CPU/523.xalancbmk_r/data/test/input/test.xml /data/benchmarks/spec2017/benchspec/CPU/523.xalancbmk_r/data/refrate/input/xalanc.xsl' #523&623
    ]
    exec_list = ['','','','','','','','cpuxalan_r','']
    warmup_list = [0, 0, 0, 0, 0, 0, 0, 0, 0]
    inst_list = [500000000, 500000000, 500000000, 500000000, 500000000, 500000000, 500000000, 500000000, 500000000]
    # create a list of all microbenchmarks

    # for filename in os.listdir('/data/benchmarks/spec2017/benchspec/CPU'):
    #     exec_name = '/data/benchmarks/spec2017/benchspec/CPU/'+filename+'/exe/'+filename.split(".")[1]+"_base.mytest-m64"
    #     if os.path.isfile(exec_name) and str(filename)[0].isdigit():
    #         bm_list.append(filename)

    jobs = []
    for i, bm in enumerate(bm_list):
        bm_exec_name = (exec_list[i] if exec_list[i] != '' else bm.split(".")[1])+"_base.mytest-m64"
        for cpu in cpu_types:
            for mem in mem_ctls:
                run = gem5Run.createSERun(
                    'microbench_tests',
                    os.getenv('M5_PATH')+'/build/X86/gem5.opt',
                    'gem5-config-dbcp/run_micro.py',
                    'results-proj-dbcp/X86/run_micro/{}/{}/{}'.format(bm,cpu,mem),
                    cpu,mem,os.path.join('/data/benchmarks/spec2017/benchspec/CPU/',bm,'exe',bm_exec_name), parameters_list[i],'--clock','4GHz', '-W', str(warmup_list[i]), '-I', str(inst_list[i]))
                jobs.append(run)

    with mp.Pool(args.N) as pool:
        pool.map(worker,jobs)

