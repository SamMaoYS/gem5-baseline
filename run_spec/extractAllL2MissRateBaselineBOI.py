# Set the absolute path in 

import numpy as np
import pandas as pd
#from matplotlib import pyplot as plt
import os
import csv

if not os.getenv("LAB_PATH"):
    print("Set Lab Path\n")
    exit(1)

benchmarks = ['510.parest_r','541.leela_r','641.leela_s','531.deepsjeng_r','631.deepsjeng_s','505.mcf_r','605.mcf_s','523.xalancbmk_r', '623.xalancbmk_s']
# for filename in os.listdir('/data/benchmarks/spec2017/benchspec/CPU'):
#     exec_name = '/data/benchmarks/spec2017/benchspec/CPU/'+filename+'/exe/'+filename.split(".")[1]+"_base.mytest-m64"
#     if os.path.isfile(exec_name) and str(filename)[0].isdigit():
#         benchmarks.append(filename)
benchmarks.sort()
datadir = os.getenv("LAB_PATH") + '/results-proj-baseline-he/X86/run_micro'
failed_benchmarks = set()

def gem5GetStat(filename, stat):
    filename = os.path.join(datadir, '', filename, 'stats.txt').replace('\\','/')
    with open(filename) as f:
        r = f.read()
        if len(r) < 10: 
            return 0.0
        if (r.find(stat) != -1) :
            start = r.find(stat) + len(stat) + 1
            end = r.find('#', start)
            return float(r[start:end])
        else:
            return float(0.0)
all_arch = ['X86']
plt_arch = ['X86']

all_mem_ctls = ['DDR3_1600_8x8', 'DDR3_2133_8x8', 'LPDDR2_S4_1066_1x32', 'HBM_1000_4H_1x64']
plt_mem_ctls = ['DDR3_1600_8x8']

all_gem5_cpus = ['Simple','DefaultO3','Minor4']
plt_gem5_cpus = ['Simple','DefaultO3']


rows = []
for bm in benchmarks: 
    for cpu in plt_gem5_cpus:
        for mem in plt_mem_ctls:
            rows.append([bm,cpu,mem,            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem, 'sim_ticks')/333,
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem, 'sim_insts'),
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem, 'sim_ops'),
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem, 'sim_ticks')/1e9,
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem, 'host_op_rate'),
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem,'system.mem_ctrl.dram.avgMemAccLat'),
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem,'system.mem_ctrl.dram.busUtil'),
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem,'system.mem_ctrl.dram.bw_total::total'),
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem,'system.mem_ctrl.dram.totBusLat'),
                                                            #memory with store
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem,'system.mem_ctrl.dram.avgWrBW'),
            gem5GetStat(datadir+"/"+bm+"/"+cpu+"/"+mem,'system.l2cache.overall_miss_rate::total')
            ])

df = pd.DataFrame(rows, columns=['benchmark','cpu', 'mem', 'cycles','instructions', 'Ops', 'Ticks','Host', 'avgmemaccesslatency','busutilit','bandwidthtotal','totalbuslatency',                                       'averagewritebandwidth', 'miss_rate'])
df['ipc'] = df['instructions']/df['cycles']
df['cpi']= 1/df['ipc']
print(df)
stat = 'miss_rate'
with open('miss_rates_baseline.csv', 'w', newline='') as csvfile:
    header = ['benchmark']
    writer = csv.writer(csvfile)
    for cpu in plt_gem5_cpus:
        for mem in plt_mem_ctls:
            header.append(mem + "-" + cpu)
    writer.writerow(header)
    for bm in benchmarks:
        row = [bm]
        for cpu in plt_gem5_cpus:
            for mem in plt_mem_ctls:
                d = df[(df['mem']==mem) & (df['benchmark']==bm) & (df['cpu']==cpu)]
                row.append(d[stat].iloc[0])
        writer.writerow(row)
        