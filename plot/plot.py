import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt

# Setup command-line argument parsing
parser = argparse.ArgumentParser(description='Generate heatmaps from multiple YAML files')
parser.add_argument('--inputs', '-i', nargs=4, type=str, required=True, help='Paths to the input YAML files (exactly 4)')
parser.add_argument('--outputheatmap', '-oh', type=str, default='./plot/HeatMaps.jpg', help='Path to the output heatmap image (default: ./plot/HeatMaps.jpg)')
parser.add_argument('--outputwaitmap', '-ow', type=str, default='./plot/WaitMaps.jpg', help='Path to the output waitmap image (default: ./plot/WaitMaps.jpg)')

# Parse arguments
args = parser.parse_args()

def generate_matrices(file_path):
    # Read the YAML file
    with open(file_path, 'r') as file:
        data = yaml.safe_load(file)
    
    # Extract grid dimensions
    width = data['width']
    height = data['height']
    
    # Initialize the matrices to record appearances and waits
    appearances_matrix = np.zeros((height, width))
    wait_matrix = np.zeros((height, width))
    
    # Process the movement data
    for agent_key in data.keys():
        if agent_key.startswith('agent'):
            previous_position = None
            for movement in data[agent_key]:
                # Adjust the coordinates
                x = max(0, min(width - 1, movement["x"] - 1))
                y = max(0, min(height - 1, height - movement["y"]))
                
                # Increment the appearance count
                appearances_matrix[y, x] += 1
                
                # Check for waiting and increment the wait count
                if previous_position == (x, y):
                    wait_matrix[y, x] += 1
                
                # Update the previous position
                previous_position = (x, y)
    
    return appearances_matrix, wait_matrix, width, height

def plot_matrix(matrix, width, height, output_path, colorbar_label):
    # Set up the figure for subplots
    fig, axs = plt.subplots(1, 4, figsize=(36, 8))

    # Create the heatmaps for each input file
    for i, ax in enumerate(axs):
        cax = ax.imshow(matrix[i], cmap='RdPu', aspect='auto', origin='lower')
        ax.tick_params(axis='x', which='both', bottom=False, top=False, labelbottom=False)
        ax.tick_params(axis='y', which='both', left=False, labelleft=False)
        ax.set_yticks(range(0, height, max(1, height // 10)))

    # Position for the colorbar
    fig.subplots_adjust(right=0.9)
    cbar_ax = fig.add_axes([0.91, 0.02, 0.01, 0.96])
    cbar = fig.colorbar(cax, cax=cbar_ax, label=colorbar_label)
    cbar.set_label(colorbar_label, fontsize=24)

    plt.tight_layout(rect=[0, 0, 0.9, 1])
    plt.savefig(output_path)

# Generate matrices and plot for each input file
appearance_matrices = []
wait_matrices = []
for input_path in args.inputs:
    appearances, waits, width, height = generate_matrices(input_path)
    appearance_matrices.append(appearances)
    wait_matrices.append(waits)

# Plotting
plot_matrix(appearance_matrices, width, height, args.outputheatmap, 'Appearance Count')
plot_matrix(wait_matrices, width, height, args.outputwaitmap, 'Wait Count')
