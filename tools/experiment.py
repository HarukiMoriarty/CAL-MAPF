import pandas as pd
import numpy as np

def analyze_cache_overhead(csv_path):
    """
    Analyze cache overhead from experimental data.
    
    Args:
        csv_path (str): Path to the CSV file containing experiment results
    
    Returns:
        dict: Dictionary containing P0, P50, P99 overhead percentages
    """
    # Read the CSV file
    df = pd.read_csv(csv_path)
    
    # Define the columns that constitute an experiment setting
    setting_columns = [
        'map_name', 'goal_generation_type', 'ngoals', 'nagents'
    ]
    
    # Function to calculate overhead percentage
    def calculate_overhead(group):
        # Get baseline (no cache) elapsed time
        baseline = group[group['cache'] == 'NONE']
        if baseline.empty:
            return None
        baseline_time = baseline['elapsed_time'].iloc[0]
        
        # Get cached times (excluding NONE)
        cached_runs = group[group['cache'] != 'NONE']
        if cached_runs.empty:
            return None
            
        # Calculate average cached time across all cache types
        avg_cached_time = cached_runs['elapsed_time'].mean()
        
        # Calculate overhead percentage
        overhead_pct = ((avg_cached_time - baseline_time) / baseline_time) * 100
        return overhead_pct
    
    # Group by experiment settings
    grouped = df.groupby(setting_columns + ['nagents'])
    overheads = []
    
    for _, group in grouped:
        print(group)
        overhead = calculate_overhead(group)
        if overhead is not None:
            overheads.append(overhead)
    
    if not overheads:
        return None
        
    # Calculate percentiles
    results = {
        'P0': np.percentile(overheads, 0),
        'P50': np.percentile(overheads, 50),
        'P99': np.percentile(overheads, 99)
    }
    
    # Also calculate per number of agents
    nagents_groups = df.groupby('nagents')
    per_agent_results = {}
    
    for nagents, group in nagents_groups:
        agent_overheads = []
        for _, setting_group in group.groupby(setting_columns):
            overhead = calculate_overhead(setting_group)
            if overhead is not None:
                agent_overheads.append(overhead)
        
        if agent_overheads:
            per_agent_results[nagents] = {
                'P0': np.percentile(agent_overheads, 0),
                'P50': np.percentile(agent_overheads, 50),
                'P99': np.percentile(agent_overheads, 99)
            }
    
    return results, per_agent_results

def main():
    csv_path = './result/result_Real.csv'  # Update with your CSV file path
    results = analyze_cache_overhead(csv_path)
    
    if results:
        overall_results, per_agent_results = results
        
        print("\nOverall Cache Overhead Analysis:")
        print(f"P0  (minimum) overhead: {overall_results['P0']:.2f}%")
        print(f"P50 (median)  overhead: {overall_results['P50']:.2f}%")
        print(f"P99           overhead: {overall_results['P99']:.2f}%")
        
        print("\nOverhead by Number of Agents:")
        for nagents, stats in sorted(per_agent_results.items()):
            print(f"\nAgents: {nagents}")
            print(f"  P0  (minimum): {stats['P0']:.2f}%")
            print(f"  P50 (median):  {stats['P50']:.2f}%")
            print(f"  P99          : {stats['P99']:.2f}%")
    else:
        print("Could not calculate overhead. Make sure the data contains both cached and non-cached (NONE) experiments.")

if __name__ == "__main__":
    main()