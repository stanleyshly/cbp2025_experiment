import os                                                                                                                                                                                                                                                                                                                                                                                   
import csv
import pandas as pd
import re
import datetime
import time
import subprocess
import multiprocessing as mp
from numpy import random
from time import sleep
import argparse
from pathlib import Path
#from scipy.stats import gmean


parser = argparse.ArgumentParser()
parser.add_argument('--trace_dir', help='path to trace directory', required= True)
parser.add_argument('--results_dir', help='path to results directory', required= True)

parser.add_argument('--predictors', help='comma-separated list of predictors to test (tage-sc-l,onebit,twobit,correlating,local,gshare)', default='tage-sc-l')
parser.add_argument('--sweep_predictors', action='store_true', help='sweep all available predictors (tage-sc-l,onebit,twobit,correlating,local,gshare)')

args = parser.parse_args()
trace_dir = Path(args.trace_dir)
results_dir = Path(args.results_dir)

# Parse predictor options

if args.sweep_predictors:
    predictors = ['tage-sc-l', 'onebit', 'twobit', 'correlating', 'local', 'gshare', 'tournament']
else:
    predictors = [p.strip() for p in args.predictors.split(',')]

# Validate predictor names
valid_predictors = ['tage-sc-l', 'onebit', 'twobit', 'correlating', 'local', 'gshare', 'tournament']
for pred in predictors:
    if pred not in valid_predictors:
        print(f"Error: '{pred}' is not a valid predictor. Valid options: {valid_predictors}")
        exit(1)

print(f"Testing predictors: {predictors}")

def get_trace_paths(start_path):
    ret_list = []
    for root, dirs, files in os.walk(start_path):
        for my_file in files:
            if(my_file.endswith('_trace.gz')):
                ret_list.append(os.path.join(root, my_file))
    return ret_list

def process_run_op(pass_status, my_trace_path, my_run_name, op_file, predictor):
    run_name_split = re.split(r"\/", my_run_name)
    wl_name = run_name_split[0]
    run_name = run_name_split[1]
    print(f'Extracting data from : {op_file} |  WL:{wl_name} | Run:{run_name}')
    exec_time = 0

    _Instr = 0
    _Cycles = 0
    _IPC = 0
    _NumBr = 0
    _MispBr = 0
    _BrPerCyc = 0
    _MispBrPerCyc = 0
    _MR = 0
    _MPKI = 0
    _CycWP = 0
    _CycWPAvg = 0
    _CycWPPKI = 0
    
    _50PercInstr = 0
    _50PercCycles = 0
    _50PercIPC = 0
    _50PercNumBr = 0
    _50PercMispBr = 0
    _50PercBrPerCyc = 0
    _50PercMispBrPerCyc = 0
    _50PercMR = 0
    _50PercMPKI = 0
    _50PercCycWP = 0
    _50PercCycWPAvg = 0
    _50PercCycWPPKI = 0

    trace_size = os.path.getsize(trace_path)/(1024 * 1024)

    pass_status_str = 'Fail'

    process_50perc_section = False
    process_100perc_section = False

    _50perc_section_header = 'DIRECT CONDITIONAL BRANCH PREDICTION MEASUREMENTS (50 Perc instructions)'
    _100perc_section_header = 'DIRECT CONDITIONAL BRANCH PREDICTION MEASUREMENTS (Full Simulation i.e. Counts Not Reset When Warmup Ends)'

    found_50perc_line_to_process = False
    found_100perc_line_to_process = False

    if pass_status:
        pass_status_str = 'Pass'
        with open(op_file, "r") as text_file:
            #for line in my_run_output.splitlines():
            for line in text_file:
                if not line.strip():
                    continue

                if('ExecTime' in line):
                    exec_time = line.strip().split()[-1]

                if(not process_50perc_section and _50perc_section_header in line):
                    process_50perc_section = True
                    process_100perc_section = False

                if(not process_100perc_section and _100perc_section_header in line):
                    process_50perc_section = False
                    process_100perc_section = True


                if process_50perc_section:
                    if (found_50perc_line_to_process):
                        curr_split_line = line.split()
                        _50PercInstr              = curr_split_line[0]
                        _50PercCycles             = curr_split_line[1]
                        _50PercIPC                = curr_split_line[2]
                        _50PercNumBr              = curr_split_line[3]
                        _50PercMispBr             = curr_split_line[4]
                        _50PercBrPerCyc           = curr_split_line[5]
                        _50PercMispBrPerCyc       = curr_split_line[6]
                        _50PercMR                 = curr_split_line[7]
                        _50PercMPKI               = curr_split_line[8]
                        _50PercCycWP              = curr_split_line[9]
                        _50PercCycWPAvg           = curr_split_line[10]
                        _50PercCycWPPKI           = curr_split_line[11]
                        process_50perc_section = False
                        found_50perc_line_to_process = False

                    if(all(x in line for x in ['Instr', 'Cycles', 'IPC', 'NumBr', 'MispBr', 'BrPerCyc', 'MispBrPerCyc', 'MR', 'MPKI', 'CycWP', 'CycWPAvg'])):
                        found_50perc_line_to_process = True

                if process_100perc_section:
                    if (found_100perc_line_to_process):
                        curr_split_line = line.split()
                        _Instr              = curr_split_line[0]
                        _Cycles             = curr_split_line[1]
                        _IPC                = curr_split_line[2]
                        _NumBr              = curr_split_line[3]
                        _MispBr             = curr_split_line[4]
                        _BrPerCyc           = curr_split_line[5]
                        _MispBrPerCyc       = curr_split_line[6]
                        _MR                 = curr_split_line[7]
                        _MPKI               = curr_split_line[8]
                        _CycWP              = curr_split_line[9]
                        _CycWPAvg           = curr_split_line[10]
                        _CycWPPKI           = curr_split_line[11]
                        process_100perc_section = False
                        found_100perc_line_to_process = False
                    if(all(x in line for x in ['Instr', 'Cycles', 'IPC', 'NumBr', 'MispBr', 'BrPerCyc', 'MispBrPerCyc', 'MR', 'MPKI', 'CycWP', 'CycWPAvg'])):
                        found_100perc_line_to_process = True

    retval = {
            'Workload'                : wl_name,
            'Run'                     : run_name,
            'Predictor'               : predictor,
            'TraceSize'               : trace_size,
            'Status'                  : pass_status_str,
            'ExecTime'                : exec_time,

            'Instr'                   : _Instr,
            'Cycles'                  : _Cycles,
            'IPC'                     : _IPC,
            'NumBr'                   : _NumBr,
            'MispBr'                  : _MispBr,
            'BrPerCyc'                : _BrPerCyc,
            'MispBrPerCyc'            : _MispBrPerCyc,
            'MR'                      : _MR,
            'MPKI'                    : _MPKI,
            'CycWP'                   : _CycWP,
            'CycWPAvg'                : _CycWPAvg,
            'CycWPPKI'                : _CycWPPKI,

            '50PercInstr'             : _50PercInstr,
            '50PercCycles'            : _50PercCycles,
            '50PercIPC'               : _50PercIPC,
            '50PercNumBr'             : _50PercNumBr,
            '50PercMispBr'            : _50PercMispBr,
            '50PercBrPerCyc'          : _50PercBrPerCyc,
            '50PercMispBrPerCyc'      : _50PercMispBrPerCyc,
            '50PercMR'                : _50PercMR,
            '50PercMPKI'              : _50PercMPKI,
            '50PercCycWP'             : _50PercCycWP,
            '50PercCycWPAvg'          : _50PercCycWPAvg,
            '50PercCycWPPKI'          : _50PercCycWPPKI,
    }
    return retval

my_traces = get_trace_paths(trace_dir)

print(f'Got {len(my_traces)} traces')

timestamp = datetime.datetime.now().strftime("%m_%d_%H-%M-%S")
if not os.path.exists(f'{results_dir}'):
    os.mkdir(results_dir)

def execute_trace(trace_pred_tuple):
    my_trace_path, predictor = trace_pred_tuple
    assert(os.path.exists(my_trace_path))
        
    run_split = re.split(r"\/", my_trace_path)
    my_wl = run_split[-2] 
    # traces/int/int_0_trace.gz
    run_name = run_split[-1].split(".")[-2]
    
    # Create directory structure with predictor subdirectory
    pred_results_dir = f'{results_dir}/{predictor}'
    if not os.path.exists(f'{pred_results_dir}/{my_wl}'):
        os.makedirs(f'{pred_results_dir}/{my_wl}', exist_ok=True)

    do_process = True
    my_run_name = f'{my_wl}/{run_name}'
    exec_cmd = f'./cbp -pred {predictor} {my_trace_path}'
    op_file = f'{pred_results_dir}/{my_wl}/{run_name}.log'
    if os.path.exists(op_file):
        #print(f"OP file:{op_file} already exists. Not running again!")
        do_process = False
    #
    pass_status = True
    if do_process:
        print(f'Begin processing run:{my_run_name} with predictor:{predictor}')
        try:
            begin_time = time.time()
            run_op = subprocess.check_output(exec_cmd, shell=True, text=True)
            end_time = time.time()
            exec_time = end_time - begin_time
            with open(op_file, "w") as text_file:
                print(f"CMD:{exec_cmd}", file=text_file)
                print(f"{run_op}", file=text_file)
                print(f"ExecTime = {exec_time}", file=text_file)
        except:
            print(f'Run: {my_run_name} with predictor:{predictor} failed')
            pass_status = False
    return(pass_status, my_trace_path, op_file, my_run_name, predictor)



if __name__ == '__main__':
    # Create trace-predictor combinations
    trace_pred_combinations = []
    for trace in my_traces:
        for predictor in predictors:
            trace_pred_combinations.append((trace, predictor))
    
    print(f'Total combinations to run: {len(trace_pred_combinations)} (traces: {len(my_traces)}, predictors: {len(predictors)})')
    
    # For parallel runs:
    with mp.Pool() as pool:
        results = pool.map(execute_trace, trace_pred_combinations)
    
    # For serial runs:
    #results = []
    #for trace_pred in trace_pred_combinations:
    #    results.append(execute_trace(trace_pred))
    
    df = pd.DataFrame(columns=['Workload', 'Run', 'Predictor', 'TraceSize', 'ExecTime', 'Instr', 'Cycles', 'IPC', 'NumBr', 'MispBr', 'BrPerCyc', 'MispBrPerCyc', 'MR', 'MPKI', 'CycWP',  'CycWPAvg', 'CycWPPKI', '50PercInstr', '50PercCycles', '50PercIPC', '50PercNumBr', '50PercMispBr', '50PercBrPerCyc', '50PercMispBrPerCyc', '50PercMR', '50PercMPKI', '50PercCycWP', '50PercCycWPAvg', '50PercCycWPPKI'])
    for my_result in results:
        pass_status = my_result[0]
        trace_path = my_result[1]
        op_file = my_result[2]
        my_run_name = my_result[3]
        predictor = my_result[4]
        run_dict = {}
        run_dict = process_run_op(pass_status, trace_path, my_run_name, op_file, predictor)
        my_df = pd.DataFrame([run_dict])
        if not df.empty:
            df = pd.concat([df, my_df], ignore_index=True)
        else:
            df = my_df.copy()
    print(df)
    df.to_csv(f'{results_dir}/results.csv', index=False)
    
    
    unique_wls = df['Workload'].unique()
    
    # Enhanced analysis with predictor comparison
    unique_predictors = df['Predictor'].unique()
    
    print('\n\n----------------------------------Aggregate Metrics Per Workload Category and Predictor----------------------------------\n')
    for my_wl in unique_wls:
        print(f'Workload: {my_wl}')
        for pred in unique_predictors:
            subset = df[(df['Workload'] == my_wl) & (df['Predictor'] == pred)]
            if not subset.empty:
                my_wl_br_misp_pki_amean = subset['50PercMPKI'].astype(float).mean()
                my_wl_cyc_wp_pki_amean = subset['50PercCycWPPKI'].astype(float).mean()
                my_wl_ipc_amean = subset['50PercIPC'].astype(float).mean()
                print(f'  {pred:<8} - BrMisPKI: {my_wl_br_misp_pki_amean:8.4f}, CycWpPKI: {my_wl_cyc_wp_pki_amean:8.4f}, IPC: {my_wl_ipc_amean:6.4f}')
        print()
    print('---------------------------------------------------------------------------------------------------------------------------')
    
    print('\n\n---------------------------------------------Aggregate Metrics Per Predictor---------------------------------------------\n')
    for pred in unique_predictors:
        subset = df[df['Predictor'] == pred]
        if not subset.empty:
            br_misp_pki_amean = subset['50PercMPKI'].astype(float).mean()
            cyc_wp_pki_amean = subset['50PercCycWPPKI'].astype(float).mean()
            ipc_amean = subset['50PercIPC'].astype(float).mean()
            print(f'{pred:<8} - Branch Misprediction PKI(BrMisPKI) AMean: {br_misp_pki_amean:8.4f}')
            print(f'{pred:<8} - Cycles On Wrong-Path PKI(CycWpPKI) AMean: {cyc_wp_pki_amean:8.4f}')
            print(f'{pred:<8} - IPC AMean: {ipc_amean:6.4f}')
            print()
    print('---------------------------------------------------------------------------------------------------------------------')
    
    # Predictor comparison summary
    if len(unique_predictors) > 1:
        print('\n\n---------------------------------------------Predictor Comparison Summary---------------------------------------------\n')
        baseline_pred = 'onebit' if 'onebit' in unique_predictors else unique_predictors[0]
        for pred in unique_predictors:
            if pred != baseline_pred:
                baseline_subset = df[df['Predictor'] == baseline_pred]
                current_subset = df[df['Predictor'] == pred]
                if not baseline_subset.empty and not current_subset.empty:
                    # Calculate instruction-weighted metrics for baseline
                    baseline_total_instrs = baseline_subset['50PercInstr'].astype(float).sum()
                    baseline_total_mispreds = baseline_subset['50PercMispBr'].astype(float).sum()
                    baseline_total_cycles = baseline_subset['50PercCycles'].astype(float).sum()
                    baseline_mpki = (baseline_total_mispreds / baseline_total_instrs) * 1000
                    baseline_ipc = baseline_total_instrs / baseline_total_cycles
                    
                    # Calculate instruction-weighted metrics for current predictor
                    current_total_instrs = current_subset['50PercInstr'].astype(float).sum()
                    current_total_mispreds = current_subset['50PercMispBr'].astype(float).sum()
                    current_total_cycles = current_subset['50PercCycles'].astype(float).sum()
                    current_mpki = (current_total_mispreds / current_total_instrs) * 1000
                    current_ipc = current_total_instrs / current_total_cycles
                    
                    mpki_improvement = ((baseline_mpki - current_mpki) / baseline_mpki) * 100
                    ipc_improvement = ((current_ipc - baseline_ipc) / baseline_ipc) * 100
                    
                    print(f'{pred} vs {baseline_pred}:')
                    print(f'  MPKI improvement: {mpki_improvement:+6.2f}%')
                    print(f'  IPC improvement:  {ipc_improvement:+6.2f}%')
                    print()
        print('-------------------------------------------------------------------------------------------------------------------')
