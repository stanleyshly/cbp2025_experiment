#!/usr/bin/env python3
"""
Example usage of the comprehensive configuration sweep system
"""

import subprocess
import os
from pathlib import Path

# Example 1: Test different gshare configurations
def test_gshare_configs():
    print("=== Testing Gshare Configurations ===")
    
    configs = [
        {'table_bits': 14, 'history_bits': 4},
        {'table_bits': 16, 'history_bits': 6},
        {'table_bits': 17, 'history_bits': 8},
    ]
    
    for i, config in enumerate(configs):
        print(f"\nConfig {i+1}: table_bits={config['table_bits']}, history_bits={config['history_bits']}")
        
        # Set environment variables
        env = os.environ.copy()
        env['GSHARE_TABLE_BITS'] = str(config['table_bits'])
        env['GSHARE_HISTORY_BITS'] = str(config['history_bits'])
        
        # Run simulation
        cmd = ['./cbp', '-pred', 'gshare', 'sample_traces/int/sample_int_trace.gz']
        result = subprocess.run(cmd, capture_output=True, text=True, env=env)
        
        if result.returncode == 0:
            # Extract MPKI from output
            lines = result.stdout.split('\n')
            for line in lines:
                if 'MPKI' in line and '%' in line:
                    parts = line.split()
                    mpki = parts[8]
                    print(f"  MPKI: {mpki}")
                    break

# Example 2: Run comprehensive sweep on sample traces
def run_sample_sweep():
    print("\n=== Running Sample Comprehensive Sweep ===")
    
    cmd = [
        'python3', 'scripts/comprehensive_sweep.py',
        '--trace_dir', 'sample_traces',
        '--results_dir', 'example_sweep_results',
        '--predictors', 'gshare', 'tournament',
        '--max_memory_kb', '128',
        '--max_configs_per_predictor', '5',
        '--parallel_jobs', '2',
        '--timeout_seconds', '60'
    ]
    
    result = subprocess.run(cmd)
    if result.returncode == 0:
        print("Sweep completed successfully!")
        print("Check example_sweep_results/ for detailed results")

if __name__ == '__main__':
    # Make sure we're in the right directory
    if not Path('./cbp').exists():
        print("Error: cbp executable not found. Run 'make' first.")
        exit(1)
    
    test_gshare_configs()
    run_sample_sweep()
