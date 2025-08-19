#!/usr/bin/env python3
"""
Comprehensive Configuration Sweep for Branch Predictors
Supports testing across the full CBP2025 trace dataset with memory constraints
"""

import argparse
import subprocess
import pandas as pd
import json
import itertools
import os
from pathlib import Path
import multiprocessing as mp
from concurrent.futures import ProcessPoolExecutor, as_completed
import time
import re
from typing import Dict, List, Any, Tuple

# Enhanced predictor configuration spaces with memory-aware settings
PREDICTOR_CONFIGS = {
    'onebit': {
        'table_bits': [10, 12, 14, 16, 17, 18, 19, 20]  # 1K to 1M entries
    },
    
    'twobit': {
        'table_bits': [10, 12, 14, 16, 17, 18, 19]  # 1K to 512K entries
    },
    
    'gshare': {
        'table_bits': [12, 14, 16, 17, 18, 19],      # 4K to 512K entries
        'history_bits': [2, 4, 6, 8, 10, 12, 14, 16] # 4 to 64K history patterns
    },
    
    'correlating': {
        'pc_bits': [8, 10, 12, 14, 16],              # PC index bits
        'history_bits': [2, 4, 6, 8, 10],           # Global history bits  
        'counter_bits': [2]                          # Keep 2-bit counters
    },
    
    'local': {
        'lht_bits': [8, 10, 12, 14],                 # Local History Table size
        'history_bits': [4, 6, 8, 10, 12],          # Local history length
        'pht_bits': [8, 10, 12, 14, 16]             # Pattern History Table size
    },
    
    'tournament': {
        'selector_bits': [10, 12, 14, 16],           # Selector table size
        'bimodal_bits': [10, 12, 14, 16],            # Bimodal table size
        'gshare_table_bits': [10, 12, 14, 16],       # Gshare table size
        'gshare_history_bits': [2, 4, 6, 8, 10]     # Gshare history length
    }
}

def estimate_memory_usage(predictor: str, config: Dict[str, Any]) -> float:
    """Estimate memory usage in KB for a configuration"""
    if predictor == 'onebit':
        return (1 << config['table_bits']) / 8.0  # 1 bit per entry
    elif predictor == 'twobit':
        return (1 << config['table_bits']) / 4.0  # 2 bits per entry
    elif predictor == 'gshare':
        return (1 << config['table_bits']) / 4.0  # 2 bits per entry
    elif predictor == 'correlating':
        total_entries = 1 << (config['pc_bits'] + config['history_bits'])
        return total_entries / 4.0  # 2 bits per entry
    elif predictor == 'local':
        lht_size_bytes = (1 << config['lht_bits']) * config['history_bits'] / 8.0
        pht_size_bytes = (1 << config['pht_bits']) / 4.0
        return lht_size_bytes + pht_size_bytes
    elif predictor == 'tournament':
        selector_size = (1 << config['selector_bits']) / 4.0
        bimodal_size = (1 << config['bimodal_bits']) / 4.0
        gshare_size = (1 << config['gshare_table_bits']) / 4.0
        return selector_size + bimodal_size + gshare_size
    else:
        return 0

def generate_smart_configs(predictor: str, max_memory_kb: int = 256, max_configs: int = 100) -> List[Dict[str, Any]]:
    """Generate smart configurations focusing on interesting design points"""
    if predictor not in PREDICTOR_CONFIGS:
        return [{}]
    
    config_space = PREDICTOR_CONFIGS[predictor]
    param_names = list(config_space.keys())
    param_values = list(config_space.values())
    
    # Generate all combinations
    all_combinations = []
    for combo in itertools.product(*param_values):
        config = dict(zip(param_names, combo))
        memory_kb = estimate_memory_usage(predictor, config)
        
        if memory_kb <= max_memory_kb:
            config['estimated_memory_kb'] = memory_kb
            all_combinations.append(config)
    
    # Sort by memory usage for better coverage
    all_combinations.sort(key=lambda x: x['estimated_memory_kb'])
    
    # Take evenly spaced samples if too many configs
    if len(all_combinations) > max_configs:
        step = len(all_combinations) // max_configs
        sampled = [all_combinations[i] for i in range(0, len(all_combinations), step)][:max_configs]
        return sampled
    
    return all_combinations

def set_environment_config(predictor: str, config: Dict[str, Any]) -> Dict[str, str]:
    """Set environment variables for predictor configuration"""
    env = os.environ.copy()
    
    if predictor == 'onebit':
        env['ONEBIT_TABLE_BITS'] = str(config['table_bits'])
    elif predictor == 'twobit':
        env['TWOBIT_TABLE_BITS'] = str(config['table_bits'])
    elif predictor == 'gshare':
        env['GSHARE_TABLE_BITS'] = str(config['table_bits'])
        env['GSHARE_HISTORY_BITS'] = str(config['history_bits'])
    elif predictor == 'correlating':
        env['CORRELATING_PC_BITS'] = str(config['pc_bits'])
        env['CORRELATING_HISTORY_BITS'] = str(config['history_bits'])
        env['CORRELATING_COUNTER_BITS'] = str(config['counter_bits'])
    elif predictor == 'local':
        env['LOCAL_LHT_BITS'] = str(config['lht_bits'])
        env['LOCAL_HISTORY_BITS'] = str(config['history_bits'])
        env['LOCAL_PHT_BITS'] = str(config['pht_bits'])
    elif predictor == 'tournament':
        env['TOURNAMENT_SELECTOR_BITS'] = str(config['selector_bits'])
        env['TOURNAMENT_BIMODAL_BITS'] = str(config['bimodal_bits'])
        env['TOURNAMENT_GSHARE_TABLE_BITS'] = str(config['gshare_table_bits'])
        env['TOURNAMENT_GSHARE_HISTORY_BITS'] = str(config['gshare_history_bits'])
    
    return env

def parse_cbp_output(output: str) -> Dict[str, float]:
    """Parse CBP simulation output to extract metrics"""
    metrics = {}
    
    # Look for the measurement line
    lines = output.strip().split('\n')
    for line in lines:
        if 'Instr' in line and 'Cycles' in line and 'IPC' in line:
            continue  # Header line
        if re.search(r'\d+\.\d+%\s+\d+\.\d+', line):  # Contains percentage and MPKI
            parts = line.split()
            try:
                # Parse the standard CBP output format
                metrics['instructions'] = int(parts[0])
                metrics['cycles'] = int(parts[1])
                metrics['ipc'] = float(parts[2])
                metrics['num_branches'] = int(parts[3])
                metrics['mispredictions'] = int(parts[4])
                metrics['branches_per_cycle'] = float(parts[5])
                metrics['mispred_per_cycle'] = float(parts[6])
                metrics['miss_rate_percent'] = float(parts[7].rstrip('%'))
                metrics['mpki'] = float(parts[8])
                metrics['cycles_wrong_path'] = int(parts[9])
                metrics['avg_wrong_path'] = float(parts[10])
                metrics['wrong_path_pki'] = float(parts[11])
                break
            except (IndexError, ValueError):
                continue
    
    return metrics

def run_single_experiment(args: Tuple) -> Dict[str, Any]:
    """Run a single predictor configuration on a trace"""
    predictor, config, trace_path, config_id, timeout_seconds = args
    
    start_time = time.time()
    
    try:
        # Set environment variables for this configuration
        env = set_environment_config(predictor, config)
        
        # Run the simulation
        cmd = ['./cbp', '-pred', predictor, str(trace_path)]
        result = subprocess.run(
            cmd, 
            capture_output=True, 
            text=True, 
            timeout=timeout_seconds,
            env=env,
            cwd=Path.cwd()
        )
        
        end_time = time.time()
        
        if result.returncode == 0:
            metrics = parse_cbp_output(result.stdout)
            
            return {
                'predictor': predictor,
                'config_id': config_id,
                'config': config,
                'trace': trace_path.name,
                'trace_category': trace_path.parent.name,
                'status': 'success',
                'runtime_seconds': end_time - start_time,
                **metrics
            }
        else:
            return {
                'predictor': predictor,
                'config_id': config_id,
                'config': config,
                'trace': trace_path.name,
                'trace_category': trace_path.parent.name,
                'status': 'failed',
                'runtime_seconds': end_time - start_time,
                'error': result.stderr
            }
    
    except subprocess.TimeoutExpired:
        return {
            'predictor': predictor,
            'config_id': config_id,
            'config': config,
            'trace': trace_path.name,
            'trace_category': trace_path.parent.name,
            'status': 'timeout',
            'runtime_seconds': timeout_seconds,
            'error': 'Simulation timed out'
        }
    except Exception as e:
        return {
            'predictor': predictor,
            'config_id': config_id,
            'config': config,
            'trace': trace_path.name,
            'trace_category': trace_path.parent.name,
            'status': 'error',
            'runtime_seconds': time.time() - start_time,
            'error': str(e)
        }

def main():
    parser = argparse.ArgumentParser(description='Comprehensive configuration sweep for branch predictors')
    parser.add_argument('--trace_dir', type=Path, required=True, help='Directory containing trace files')
    parser.add_argument('--results_dir', type=Path, required=True, help='Directory to save results')
    parser.add_argument('--predictors', nargs='+', required=True,
                       choices=['onebit', 'twobit', 'gshare', 'correlating', 'local', 'tournament'],
                       help='Predictors to test')
    parser.add_argument('--max_memory_kb', type=int, default=256, help='Maximum memory per predictor in KB')
    parser.add_argument('--max_configs_per_predictor', type=int, default=50, help='Limit configs per predictor')
    parser.add_argument('--parallel_jobs', type=int, default=mp.cpu_count()//2, help='Number of parallel jobs')
    parser.add_argument('--trace_categories', nargs='+', help='Trace categories to test (fp, int, web, etc.)')
    parser.add_argument('--timeout_seconds', type=int, default=300, help='Timeout per simulation in seconds')
    parser.add_argument('--sample_traces', type=int, help='Sample N traces per category')
    
    args = parser.parse_args()
    
    # Create results directory
    args.results_dir.mkdir(parents=True, exist_ok=True)
    
    # Find trace files
    trace_files = []
    if args.trace_categories:
        for category in args.trace_categories:
            category_path = args.trace_dir / category
            if category_path.exists():
                traces = list(category_path.glob('*.gz'))
                if args.sample_traces and len(traces) > args.sample_traces:
                    traces = traces[:args.sample_traces]
                trace_files.extend(traces)
    else:
        trace_files = list(args.trace_dir.glob('**/*.gz'))
        if args.sample_traces and len(trace_files) > args.sample_traces:
            trace_files = trace_files[:args.sample_traces]
    
    print(f"Found {len(trace_files)} trace files")
    
    # Generate experiments
    experiments = []
    all_configs = {}
    
    for predictor in args.predictors:
        configs = generate_smart_configs(predictor, args.max_memory_kb, args.max_configs_per_predictor)
        all_configs[predictor] = configs
        
        print(f"{predictor}: {len(configs)} configurations (memory: {args.max_memory_kb}KB limit)")
        
        for i, config in enumerate(configs):
            config_id = f"{predictor}_config_{i:03d}"
            for trace_file in trace_files:
                experiments.append((predictor, config, trace_file, config_id, args.timeout_seconds))
    
    print(f"Total experiments: {len(experiments)}")
    print(f"Estimated runtime: {len(experiments) * 30 / args.parallel_jobs / 60:.1f} minutes")
    
    # Save configuration summary
    with open(args.results_dir / 'config_summary.json', 'w') as f:
        json.dump(all_configs, f, indent=2)
    
    # Run experiments in parallel
    results = []
    completed = 0
    
    with ProcessPoolExecutor(max_workers=args.parallel_jobs) as executor:
        # Submit all jobs
        future_to_exp = {executor.submit(run_single_experiment, exp): exp for exp in experiments}
        
        # Collect results as they complete
        for future in as_completed(future_to_exp):
            result = future.result()
            results.append(result)
            completed += 1
            
            if completed % 50 == 0:
                print(f"Completed {completed}/{len(experiments)} experiments ({100*completed/len(experiments):.1f}%)")
    
    # Save results
    results_df = pd.DataFrame(results)
    results_file = args.results_dir / 'comprehensive_sweep_results.csv'
    results_df.to_csv(results_file, index=False)
    
    # Generate summary statistics
    if not results_df.empty:
        success_df = results_df[results_df['status'] == 'success']
        
        if not success_df.empty:
            summary = {
                'total_experiments': len(results),
                'successful_experiments': len(success_df),
                'failed_experiments': len(results_df[results_df['status'] == 'failed']),
                'timeout_experiments': len(results_df[results_df['status'] == 'timeout']),
                'average_runtime': success_df['runtime_seconds'].mean(),
                'best_configs_by_predictor': {}
            }
            
            # Find best configuration for each predictor
            for predictor in args.predictors:
                pred_df = success_df[success_df['predictor'] == predictor]
                if not pred_df.empty:
                    # Group by config and calculate average MPKI
                    config_performance = pred_df.groupby('config_id')['mpki'].mean().sort_values()
                    best_config_id = config_performance.index[0]
                    best_mpki = config_performance.iloc[0]
                    
                    # Get the actual config
                    best_config = pred_df[pred_df['config_id'] == best_config_id]['config'].iloc[0]
                    
                    summary['best_configs_by_predictor'][predictor] = {
                        'config_id': best_config_id,
                        'config': best_config,
                        'average_mpki': best_mpki
                    }
            
            with open(args.results_dir / 'summary.json', 'w') as f:
                json.dump(summary, f, indent=2)
    
    print(f"\nResults saved to {results_file}")
    print(f"Configuration summary saved to {args.results_dir / 'config_summary.json'}")
    print(f"Summary statistics saved to {args.results_dir / 'summary.json'}")

if __name__ == '__main__':
    main()
