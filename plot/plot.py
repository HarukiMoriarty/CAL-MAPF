import yaml
import numpy as np
import matplotlib.pyplot as plt

# Path to the YAML file
file_path = './plot/vis.yaml'

# Read the YAML file
with open(file_path, 'r') as file:
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
plt.savefig("HeatMap.jpg")