import pandas as pd
import matplotlib.pyplot as plt
import os

# Change to script directory to ensure correct path
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

# Read the CSV file
data = pd.read_csv('Output/oscillator_output.csv', delimiter=',')

# Extract time and angle columns
time = data['Time']
angle = data['Angle']

# Create the plot
plt.figure(figsize=(12, 6))
plt.plot(time, angle, linewidth=0.8, color='blue')
plt.xlabel('Time (s)', fontsize=12)
plt.ylabel('Angle (rad)', fontsize=12)
plt.title('Driven Damped Oscillator: Angle vs Time', fontsize=14, fontweight='bold')
plt.grid(True, alpha=0.3)
plt.tight_layout()

# Save the plot
plt.savefig('Output/angle_vs_time.png', dpi=300)
print("Plot saved to Output/angle_vs_time.png")

# Display the plot
plt.show()
