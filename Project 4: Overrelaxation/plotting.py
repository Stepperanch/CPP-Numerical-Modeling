import numpy as np
import matplotlib.pyplot as plt

try:
    import plotly.graph_objects as go
    PLOTLY_AVAILABLE = True
except ImportError:
    PLOTLY_AVAILABLE = False
    print("Warning: plotly not installed. Interactive HTML export disabled.")
    print("Install with: pip install plotly")

"""
Header

This script loads the 3D potential field data from 'output.npz' and creates
 a high-resolution 3D surface plot of the center slice (z = N/2).
 The plot is interactive, allowing you to rotate it to your desired angle
 before saving. The final image is saved as 'potential_3D_center_slice.png'
 with a resolution of 300 DPI.

Author: Nels Buhrley
Date: 2024-06-01
"""

# ===== Interactive HTML Export =====
# Set to True to save an interactive HTML file (requires plotly)
SAVE_INTERACTIVE_HTML = True
INTERACTIVE_HTML_OUTPUT = 'output/potential_3d_interactive.html'

# Load data
data = np.load("output/output.npz")
potential = data["potential"]
center = potential.shape[0] // 2

# Create high-resolution figure
fig = plt.figure(figsize=(14, 10))
ax = fig.add_subplot(111, projection='3d')

# Create meshgrid for full resolution
X = np.linspace(-0.5, 0.5, potential.shape[0])
Y = np.linspace(-0.5, 0.5, potential.shape[1])
X_mesh, Y_mesh = np.meshgrid(X, Y)

# Get center slice data
Z_data = potential[center, :, :]

# Save interactive HTML if enabled
def save_interactive_html(output_path):
    """Save an interactive 3D surface plot as an HTML file using Plotly with Z-axis slider."""
    if not PLOTLY_AVAILABLE:
        print("Skipping interactive HTML export: plotly is not installed.")
        return

    z_indices = range(potential.shape[0])

    # Initial figure with center slice
    fig_plotly = go.Figure(data=[go.Surface(
        x=X_mesh,
        y=Y_mesh,
        z=potential[center, :, :],
        colorscale='Viridis',
        colorbar=dict(title='Potential (V)'),
        opacity=0.9,
        cmin=potential.min(),
        cmax=potential.max()
    )])

    # Create slider steps that directly update the data (no animation frames)
    slider_steps = []
    for z_idx in z_indices:
        z_position = X[z_idx]  # Actual z position in meters
        step = dict(
            method="update",
            args=[
                {"z": [potential[z_idx, :, :]]},  # Update the surface z-data
                {"title": f'3D Potential Field at Z = {z_position:.4f} m (slice {z_idx}/{len(z_indices)-1})'}
            ],
            label=str(z_idx)
        )
        slider_steps.append(step)

    sliders = [dict(
        active=center,
        yanchor="top",
        y=0,
        xanchor="left",
        x=0.1,
        currentvalue=dict(
            prefix="Z-Slice: ",
            visible=True,
            xanchor="right"
        ),
        pad=dict(b=10, t=50),
        len=0.8,
        steps=slider_steps
    )]

    fig_plotly.update_layout(
        title=f'3D Potential Field at Z = {X[center]:.4f} m (slice {center}/{len(z_indices)-1})',
        scene=dict(
            xaxis_title='X Position (m)',
            yaxis_title='Y Position (m)',
            zaxis_title='Potential (V)',
            camera=dict(eye=dict(x=1.5, y=1.5, z=1.0)),
        ),
        sliders=sliders,
        autosize=True,
        margin=dict(l=10, r=10, t=30, b=80),
        template="plotly_white"
    )

    fig_plotly.write_html(output_path, include_plotlyjs='cdn', full_html=True)
    print(f"Interactive HTML plot with Z-axis slider saved to {output_path}")
    print(f"  - {len(z_indices)} z-slices available")
    print(f"  - Starts at center slice (z={center})")
    print(f"  - Use slider to manually navigate through z-axis")

if SAVE_INTERACTIVE_HTML:
    save_interactive_html(INTERACTIVE_HTML_OUTPUT)

# Plot center slice at full resolution
surf = ax.plot_surface(X_mesh, Y_mesh, Z_data, cmap='viridis',
                       vmin=potential.min(), vmax=potential.max(),
                       antialiased=True, alpha=0.9)

ax.set_xlabel('X Position (m)', fontsize=12)
ax.set_ylabel('Y Position (m)', fontsize=12)
ax.set_zlabel('Potential (V)', fontsize=12)
ax.set_title(f'3D Potential Field at Z = {center} (Center Slice)', fontsize=14)

# Add colorbar
cbar = fig.colorbar(surf, ax=ax, shrink=0.5, aspect=10)
cbar.set_label('Potential (V)', fontsize=12)

# Set viewing angle
ax.view_init(elev=25, azim=45)

# Display (rotate to desired angle, then close window)
print("Rotate the plot to your desired angle, then close the window to save...")
plt.show()

# Save high-resolution image with the angle you chose
fig.savefig('output/potential_3D_center_slice.png', dpi=300, bbox_inches='tight')
print(f"Saved 3D plot at z={center} to 'output/potential_3D_center_slice.png'")