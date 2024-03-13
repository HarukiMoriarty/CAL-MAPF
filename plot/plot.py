import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt

# Setup command-line argument parsing
parser = argparse.ArgumentParser(description='Generate a heatmap from a YAML file')
parser.add_argument('--input', '-i', type=str, default='./plot/vis.yaml', help='Path to the input YAML file (default: ./plot/vis.yaml)')
parser.add_argument('--output', '-o', type=str, default='./plot/HeatMap.jpg', help='Path to the output heatmap image (default: ./plot/HeatMap.jpg)')

# Parse arguments
args = parser.parse_args()

# Read the YAML file
with open(args.input, 'r') as file:
    data = yaml.safe_load(file)

# Extract grid dimensions
width = data['width']
height = data['height']

# Initialize the matrix to record appearances
appearances_matrix = np.zeros((height, width))

# Assuming the data for movements is under a key like 'agent0' or similar
for agent_key in data.keys():
    if agent_key.startswith('agent'):
        for movement in data[agent_key]:
            # Adjust the coordinates and increment the appearance count
            x = max(0, min(width - 1, movement["x"] - 1))
            y = max(0, min(height - 1, height - movement["y"]))
            appearances_matrix[y, x] += 1

plt.figure(figsize=(10, 6))  # Adjust figure size as needed
plt.imshow(appearances_matrix, cmap='RdPu', aspect='auto', origin='lower')
plt.colorbar(label='Appearance Count')
plt.title('Agent Movement Heatmap')
plt.xlabel('Height')
plt.ylabel('Width')
plt.xticks(range(0, width, max(1, width // 10)))  # Adjust the step based on your grid width
plt.yticks(range(0, height, max(1, height // 10)))  # Adjust the step based on your grid height
plt.savefig(args.output)