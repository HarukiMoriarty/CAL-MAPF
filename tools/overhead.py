import pandas as pd
import numpy as np
import argparse
import os

def analyze_cache_overhead(input_csv, output_csv):
    """
    Analyze cache overhead from experimental data and output detailed results to CSV.
    
    Args:
        input_csv (str): Path to the input CSV file containing experiment results
        output_csv (str): Path to save the output CSV with overhead analysis
    """
    # Read the CSV file
    df = pd.read_csv(input_csv)
    
    # Define the columns that constitute an experiment setting
    setting_columns = [
        'map_name', 'goal_generation_type', 'ngoals', 'nagents'
    ]
    
    # Prepare results storage
    results = []
    
    # Group by experiment settings and calculate overheads
    grouped = df.groupby(setting_columns)
    
    for setting_values, group in grouped:
        # Check if we have both cached and non-cached data
        if 'NONE' not in group['cache'].values:
            continue
            
        # Get baseline (no cache) elapsed time
        baseline_time = group[group['cache'] == 'NONE']['elapsed_time'].iloc[0]
        
        # Get cached times (excluding NONE)
        cached_times = group[group['cache'] != 'NONE']['elapsed_time']
        
        if cached_times.empty:
            continue
            
        # Calculate average cached time
        avg_cached_time = cached_times.mean()
        
        # Calculate overhead percentage
        overhead_pct = ((avg_cached_time - baseline_time) / baseline_time) * 100
        
        # Create result dictionary with all setting values
        result = dict(zip(setting_columns, setting_values))
        result.update({
            'baseline_time': baseline_time,
            'avg_cached_time': avg_cached_time,
            'overhead_percentage': overhead_pct
        })
        
        results.append(result)
    
    if not results:
        print("Could not calculate overhead. Make sure the data contains both cached and non-cached (NONE) experiments.")
        return None
    
    # Convert results to DataFrame and save to CSV
    results_df = pd.DataFrame(results)
    
    # Calculate overall percentiles
    p0 = np.percentile(results_df['overhead_percentage'], 0)
    p50 = np.percentile(results_df['overhead_percentage'], 50)
    p99 = np.percentile(results_df['overhead_percentage'], 99)
    
    # Print summary statistics
    print("\nCache Overhead Analysis Results:")
    print(f"P0  (minimum) overhead: {p0:.2f}%")
    print(f"P50 (median)  overhead: {p50:.2f}%")
    print(f"P99           overhead: {p99:.2f}%")
    
    # Sort results by map_name and nagents for better readability
    results_df = results_df.sort_values(['map_name', 'nagents'])
    
    # Save to CSV
    results_df.to_csv(output_csv, index=False, float_format='%.2f')
    print(f"\nDetailed results saved to: {output_csv}")
    
    return {
        'P0': p0,
        'P50': p50,
        'P99': p99
    }

def main():
    parser = argparse.ArgumentParser(description='Analyze cache overhead from experimental results.')
    parser.add_argument('input_csv', help='Path to the input CSV file')
    parser.add_argument('--output', '-o', 
                      help='Path to save the output CSV (default: overhead_analysis.csv)',
                      default='overhead_analysis.csv')
    
    args = parser.parse_args()
    
    # Check if input file exists
    if not os.path.exists(args.input_csv):
        print(f"Error: Input file '{args.input_csv}' does not exist.")
        return
    
    # Create output directory if it doesn't exist
    output_dir = os.path.dirname(args.output)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Run analysis
    analyze_cache_overhead(args.input_csv, args.output)

if __name__ == "__main__":
    main()