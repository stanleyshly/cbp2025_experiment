#!/usr/bin/env python3
"""
Configuration Sweep Script for Branch Predictors
Supports testing wide variety of configurations across full trace dataset
"""

import argparse
import subprocess
import pandas as pd
import json
import itertools
from pathlib import Path
import multiprocessing as mp
from typing import Dict, List, Any

# Predictor configuration spaces
PREDICTOR_CONFIGS = {
    'onebit': {
        'table_bits': [12, 14, 16, 18, 20]  # 4K to 1M entries
    },
    
    'twobit': {
        'table_bits': [12, 14, 16, 18, 19]  # 4K to 512K entries (2 bits each)
    },
    
    'gshare': {
        'table_bits': [14, 16, 17, 18],      # 16K to 256K entries
        'history_bits': [2, 4, 6, 8, 10, 12] # 4 to 4096 history patterns
    },
    
    'correlating': {
        'pc_bits': [10, 12, 14, 16],         # PC index bits
        'history_bits': [4, 6, 8, 10],       # Global history bits  
        'counter_bits': [2]                   # Keep 2-bit counters
    },
    
    'local': {
        'lht_bits': [10, 12, 14],            # Local History Table size
        'history_bits': [6, 8, 10],          # Local history length
        'pht_bits': [10, 12, 14]             # Pattern History Table size
    },
    
    'tournament': {
        'selector_bits': [12, 14, 16],       # Selector table size
        'bimodal_bits': [12, 14, 16],        # Bimodal table size
        'gshare_table_bits': [12, 14, 16],   # Gshare table size
        'gshare_history_bits': [4, 6, 8]    # Gshare history length
    }
}

def generate_config_combinations(predictor: str) -> List[Dict[str, Any]]:
    """Generate all combinations of parameters for a predictor"""
    if predictor not in PREDICTOR_CONFIGS:
        return [{}]  # Default config
    
    config_space = PREDICTOR_CONFIGS[predictor]
    param_names = list(config_space.keys())
    param_values = list(config_space.values())
    
    combinations = []
    for combo in itertools.product(*param_values):
        config = dict(zip(param_names, combo))
        combinations.append(config)
    
    return combinations

def estimate_memory_usage(predictor: str, config: Dict[str, Any]) -> int:
    """Estimate memory usage in KB for a configuration"""
    if predictor == 'onebit':
        return (1 << config['table_bits']) // 8  # 1 bit per entry, convert to KB
    elif predictor == 'twobit':
        return (1 << config['table_bits']) // 4  # 2 bits per entry, convert to KB
    elif predictor == 'gshare':
        return (1 << config['table_bits']) // 4  # 2 bits per entry
    elif predictor == 'correlating':
        total_entries = 1 << (config['pc_bits'] + config['history_bits'])
        return total_entries // 4  # 2 bits per entry
    elif predictor == 'local':
        lht_size = (1 << config['lht_bits']) * config['history_bits'] // 8  # LHT in bytes
        pht_size = (1 << config['pht_bits']) // 4  # PHT in bytes
        return lht_size + pht_size
    elif predictor == 'tournament':
        selector_size = (1 << config['selector_bits']) // 4
        bimodal_size = (1 << config['bimodal_bits']) // 4
        gshare_size = (1 << config['gshare_table_bits']) // 4
        return selector_size + bimodal_size + gshare_size
    else:
        return 0

def filter_configs_by_memory(configs: List[Dict[str, Any]], predictor: str, max_memory_kb: int = 256) -> List[Dict[str, Any]]:
    """Filter configurations by memory constraint"""
    filtered = []
    for config in configs:
        memory_kb = estimate_memory_usage(predictor, config)
        if memory_kb <= max_memory_kb:
            config['estimated_memory_kb'] = memory_kb
            filtered.append(config)
    return filtered

def create_modified_predictor_files(predictor: str, config: Dict[str, Any], config_id: str):
    """Create modified predictor files with hardcoded configuration"""
    # This would involve creating modified .cc files with hardcoded parameters
    # For now, we'll assume the existing interface files can be modified
    
    if predictor == 'gshare':
        # Modify cond_branch_predictor_interface.cc with custom config
        interface_file = Path('cond_branch_predictor_interface.cc')
        with open(interface_file, 'r') as f:
            content = f.read()
        
        # Replace the gshare init line
        old_line = "gshare_predictor_init(17, 4);"
        new_line = f"gshare_predictor_init({config['table_bits']}, {config['history_bits']});"
        content = content.replace(old_line, new_line)
        
        # Write to temporary file
        temp_file = f'cond_branch_predictor_interface_{config_id}.cc'
        with open(temp_file, 'w') as f:
            f.write(content)
        
        return temp_file
    
    # Similar logic for other predictors...
    return None

def run_single_experiment(args):
    """Run a single predictor configuration on a trace"""
    predictor, config, trace_path, results_dir, config_id = args
    
    # Create config-specific executable if needed
    # For simplicity, we'll modify the interface files temporarily
    
    try:
        # Run the simulation
        cmd = ['./cbp', '-pred', predictor, str(trace_path)]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        
        if result.returncode == 0:
            # Parse results and save
            # Extract MPKI, IPC, etc. from output
            lines = result.stdout.strip().split('\n')
            for line in lines:
                if 'MPKI' in line and 'MR' in line:
                    # Parse the measurement line
                    parts = line.split()
                    # Extract metrics...
                    pass
            
            return {
                'predictor': predictor,
                'config_id': config_id,
                'config': config,
                'trace': trace_path.name,
                'status': 'success',
                'output': result.stdout
            }
        else:
            return {
                'predictor': predictor,
                'config_id': config_id,
                'config': config,
                'trace': trace_path.name,
                'status': 'failed',
                'error': result.stderr
            }
    
    except Exception as e:
        return {
            'predictor': predictor,
            'config_id': config_id,
            'config': config,
            'trace': trace_path.name,
            'status': 'error',
            'error': str(e)
        }

def main():
    parser = argparse.ArgumentParser(description='Configuration sweep for branch predictors')
    parser.add_argument('--trace_dir', type=Path, required=True, help='Directory containing trace files')
    parser.add_argument('--results_dir', type=Path, required=True, help='Directory to save results')
    parser.add_argument('--predictors', nargs='+', help='Predictors to test', 
                       choices=['onebit', 'twobit', 'gshare', 'correlating', 'local', 'tournament'])
    parser.add_argument('--max_memory_kb', type=int, default=256, help='Maximum memory per predictor in KB')
    parser.add_argument('--max_configs_per_predictor', type=int, default=50, help='Limit configs per predictor')
    parser.add_argument('--parallel_jobs', type=int, default=mp.cpu_count()//2, help='Number of parallel jobs')
    parser.add_argument('--trace_subset', help='Trace subset (fp, int, web, etc.)')
    
    args = parser.parse_args()
    
    # Create results directory
    args.results_dir.mkdir(parents=True, exist_ok=True)
    
    # Find trace files
    if args.trace_subset:
        trace_pattern = f'{args.trace_subset}/*.gz'
    else:
        trace_pattern = '**/*.gz'
    
    trace_files = list(args.trace_dir.glob(trace_pattern))
    print(f"Found {len(trace_files)} trace files")
    
    # Generate all experiments
    experiments = []
    for predictor in args.predictors:
        configs = generate_config_combinations(predictor)
        configs = filter_configs_by_memory(configs, predictor, args.max_memory_kb)
        
        # Limit number of configs if requested
        if len(configs) > args.max_configs_per_predictor:
            configs = configs[:args.max_configs_per_predictor]
        
        print(f"{predictor}: {len(configs)} configurations")
        
        for i, config in enumerate(configs):
            config_id = f"{predictor}_config_{i:03d}"
            for trace_file in trace_files:
                experiments.append((predictor, config, trace_file, args.results_dir, config_id))
    
    print(f"Total experiments: {len(experiments)}")
    
    # Run experiments in parallel
    with mp.Pool(args.parallel_jobs) as pool:
        results = pool.map(run_single_experiment, experiments)
    
    # Save results
    results_df = pd.DataFrame(results)
    results_file = args.results_dir / 'config_sweep_results.csv'
    results_df.to_csv(results_file, index=False)
    
    # Save configuration summary
    config_summary = {}
    for predictor in args.predictors:
        configs = generate_config_combinations(predictor)
        configs = filter_configs_by_memory(configs, predictor, args.max_memory_kb)
        config_summary[predictor] = configs[:args.max_configs_per_predictor]
    
    with open(args.results_dir / 'config_summary.json', 'w') as f:
        json.dump(config_summary, f, indent=2)
    
    print(f"Results saved to {results_file}")

if __name__ == '__main__':
    main()
