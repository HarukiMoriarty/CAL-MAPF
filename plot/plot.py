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

def generate_appearance_matrix(file_path):
    # Read the YAML file
    with open(file_path, 'r') as file:
        data = yaml.safe_load(file)
    
    # Extract grid dimensions
    width = data['width']
    height = data['height']
    
    # Initialize the matrix to record appearances
    appearances_matrix = np.zeros((height, width))
    
    # Process the movement data
    for agent_key in data.keys():
        if agent_key.startswith('agent'):
            for movement in data[agent_key]:
                # Adjust the coordinates and increment the appearance count
                x = max(0, min(width - 1, movement["x"] - 1))
                y = max(0, min(height - 1, height - movement["y"]))
                appearances_matrix[y, x] += 1
    return appearances_matrix, width, height

def generate_wait_matrix(file_path):
    # Read the YAML file
    with open(file_path, 'r') as file:
        data = yaml.safe_load(file)
    
    # Extract grid dimensions
    width = data['width']
    height = data['height']
    
    # Initialize the matrix to record appearances
    appearances_matrix = np.zeros((height, width))
    
    # Process the movement data
    for agent_key in data.keys():
        if agent_key.startswith('agent'):
            past = (0, 0)
            for movement in data[agent_key]:
                # Adjust the coordinates and increment the appearance count
                x = max(0, min(width - 1, movement["x"] - 1))
                y = max(0, min(height - 1, height - movement["y"]))
                if (y, x) == past:
                    appearances_matrix[y, x] += 1
                past = (y, x)
    return appearances_matrix, width, height

# Set up the figure for subplots
fig, axs = plt.subplots(1, 4, figsize=(36, 8))  # Adjust figure size as needed

# Create the heatmaps for each input file
for i, input_path in enumerate(args.inputs):
    appearances_matrix, width, height = generate_appearance_matrix(input_path)
    ax = axs[i]
    cax = ax.imshow(appearances_matrix, cmap='RdPu', aspect='auto', origin='lower')
    # Disable x-axis tick labels
    ax.tick_params(axis='x', which='both', bottom=False, top=False, labelbottom=False)
    # Optionally, set y-axis labels only for the first subplot
    ax.tick_params(axis='y', which='both', left=False, labelleft=False)
    ax.set_yticks(range(0, height, max(1, height // 10)))

# Position for the colorbar
fig.subplots_adjust(right=0.9)  # Adjust the right side to make room for the colorbar
cbar_ax = fig.add_axes([0.91, 0.02, 0.01, 0.96])  # Adjust width and position of colorbar
fig.colorbar(cax, cax=cbar_ax, label='Appearance Count')

plt.tight_layout(rect=[0, 0, 0.9, 1])  # Adjust the layout
plt.savefig(args.outputheatmap)

# Waitmap
fig, axs = plt.subplots(1, 4, figsize=(36, 8))  # Adjust figure size as needed

# Create the heatmaps for each input file
for i, input_path in enumerate(args.inputs):
    appearances_matrix, width, height = generate_wait_matrix(input_path)
    ax = axs[i]
    cax = ax.imshow(appearances_matrix, cmap='RdPu', aspect='auto', origin='lower')
    # Disable x-axis tick labels
    ax.tick_params(axis='x', which='both', bottom=False, top=False, labelbottom=False)
    # Optionally, set y-axis labels only for the first subplot
    ax.tick_params(axis='y', which='both', left=False, labelleft=False)
    ax.set_yticks(range(0, height, max(1, height // 10)))

# Position for the colorbar
fig.subplots_adjust(right=0.9)  # Adjust the right side to make room for the colorbar
cbar_ax = fig.add_axes([0.91, 0.02, 0.01, 0.96])  # Adjust width and position of colorbar
fig.colorbar(cax, cax=cbar_ax, label='Appearance Count')

plt.tight_layout(rect=[0, 0, 0.9, 1])  # Adjust the layout
plt.savefig(args.outputwaitmap)

