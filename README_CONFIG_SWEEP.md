# Configuration Sweep System for Branch Predictors

This system allows you to test a wide variety of predictor configurations across the entire CBP2025 trace dataset.

## 🎯 Key Features

- **Dynamic Configuration**: Set predictor parameters via environment variables
- **Memory-Aware**: Automatically filters configurations by memory constraints
- **Parallel Execution**: Run multiple experiments simultaneously
- **Comprehensive Coverage**: Test across full trace dataset
- **Smart Sampling**: Intelligently sample configuration space

## 🔧 Quick Start

### 1. Manual Configuration Testing

```bash
# Test gshare with custom parameters
GSHARE_TABLE_BITS=16 GSHARE_HISTORY_BITS=8 ./cbp -pred gshare trace.gz

# Test tournament with smaller tables
TOURNAMENT_SELECTOR_BITS=12 TOURNAMENT_BIMODAL_BITS=12 ./cbp -pred tournament trace.gz

# View configuration being used
PRINT_PREDICTOR_CONFIG=1 ./cbp -pred gshare trace.gz
```

### 2. Comprehensive Sweep

```bash
# Basic sweep on sample traces
python scripts/comprehensive_sweep.py \
  --trace_dir sample_traces \
  --results_dir my_results \
  --predictors gshare tournament local \
  --max_memory_kb 256 \
  --max_configs_per_predictor 20

# Full CBP2025 dataset sweep
python scripts/comprehensive_sweep.py \
  --trace_dir traces \
  --results_dir full_sweep_results \
  --predictors gshare tournament local correlating \
  --max_memory_kb 256 \
  --max_configs_per_predictor 50 \
  --trace_categories fp int \
  --parallel_jobs 8
```

## 🎮 Environment Variables

### Onebit Predictor
- `ONEBIT_TABLE_BITS` (default: 17)

### Twobit Predictor  
- `TWOBIT_TABLE_BITS` (default: 17)

### Gshare Predictor
- `GSHARE_TABLE_BITS` (default: 17)
- `GSHARE_HISTORY_BITS` (default: 4)

### Correlating Predictor
- `CORRELATING_PC_BITS` (default: 14)
- `CORRELATING_HISTORY_BITS` (default: 4)
- `CORRELATING_COUNTER_BITS` (default: 2)

### Local Predictor
- `LOCAL_LHT_BITS` (default: 14)
- `LOCAL_HISTORY_BITS` (default: 6)
- `LOCAL_PHT_BITS` (default: 12)

### Tournament Predictor
- `TOURNAMENT_SELECTOR_BITS` (default: 14)
- `TOURNAMENT_BIMODAL_BITS` (default: 14)
- `TOURNAMENT_GSHARE_TABLE_BITS` (default: 14)
- `TOURNAMENT_GSHARE_HISTORY_BITS` (default: 6)

## 📊 Configuration Spaces

The sweep system tests these parameter ranges:

### Gshare
- Table bits: 12-19 (4K to 512K entries)
- History bits: 2-16 (4 to 64K patterns)
- Memory usage: 1KB to 128KB

### Tournament
- Selector bits: 10-16 (1K to 64K entries)
- Component predictor sizes: 10-16 bits
- Combined memory: 3KB to 192KB

### Local
- LHT bits: 8-14 (256 to 16K entries)
- History bits: 4-12 (16 to 4K patterns)
- PHT bits: 8-16 (256 to 64K entries)

## 📈 Results Analysis

The sweep generates:

1. **comprehensive_sweep_results.csv**: Raw results for all experiments
2. **config_summary.json**: All tested configurations
3. **summary.json**: Best configurations per predictor

### Example Analysis

```python
import pandas as pd

# Load results
df = pd.read_csv('results/comprehensive_sweep_results.csv')

# Find best gshare configuration
gshare_results = df[df['predictor'] == 'gshare']
best_gshare = gshare_results.groupby('config_id')['mpki'].mean().idxmin()
print(f"Best gshare config: {best_gshare}")

# Compare predictors
predictor_performance = df.groupby('predictor')['mpki'].mean().sort_values()
print(predictor_performance)
```

## 🚀 Advanced Usage

### Memory Constraint Sweep
```bash
# Test different memory budgets
for memory in 64 128 256 512; do
  python scripts/comprehensive_sweep.py \
    --trace_dir traces \
    --results_dir "results_${memory}kb" \
    --predictors gshare tournament \
    --max_memory_kb $memory \
    --max_configs_per_predictor 30
done
```

### Workload-Specific Analysis
```bash
# Test FP vs INT workloads separately
python scripts/comprehensive_sweep.py \
  --trace_dir traces \
  --results_dir fp_optimized \
  --predictors tournament local \
  --trace_categories fp \
  --max_configs_per_predictor 100

python scripts/comprehensive_sweep.py \
  --trace_dir traces \
  --results_dir int_optimized \
  --predictors gshare correlating \
  --trace_categories int \
  --max_configs_per_predictor 100
```

## 💡 Tips

1. **Start Small**: Use sample_traces for initial exploration
2. **Memory Matters**: Constrain memory for fair comparisons
3. **Parallel Jobs**: Use CPU_COUNT/2 for optimal performance
4. **Timeout**: Set appropriate timeouts for large traces
5. **Sampling**: Use --sample_traces to limit experiments

## 🔍 Example Results

```
Best Configurations (256KB memory budget):
- Gshare: table_bits=17, history_bits=6 → 2.1 MPKI
- Tournament: selector=14, bimodal=14, gshare_table=14, gshare_hist=4 → 2.8 MPKI
- Local: lht=12, history=8, pht=14 → 2.4 MPKI
```
