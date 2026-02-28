"""
plotting.py — Visualisation script for the 3D Ising Model simulation output.

Author : Nels Buhrley
Date   : 2026-02-17

Overview
--------
Loads the simulation results produced by the C++ Ising model program
(saved as a NumPy .npz archive) and generates two types of plots:

  1. 3D surface plot  - average magnetisation as a function of temperature T
                        and external magnetic field h.  Can be rendered from
                        multiple viewing angles automatically.

  2. 2D contour map   - the same data projected onto the T-h plane, with
                        filled contours and labelled iso-magnetisation lines.

Both figures are saved as high-resolution PNG files in the output directory.

Expected input file
-------------------
  output/Out_3/ising_results.npz
    Arrays:
      temperatures      - 1-D array of temperature values
      magnetic_fields   - 1-D array of external field values
      avg_magnetization - flattened 2-D array, shape (n_h, n_T)
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import sys

# ── Viewing-Angle Configuration ──────────────────────────────────────────────
# These parameters control the camera position for the 3D surface plot.
# Elevation: angle above the xy-plane (degrees).  0 = horizontal, 90 = top-down.
VIEW_ELEVATION = 25
# Azimuth: rotation around the vertical (z) axis (degrees).
VIEW_AZIMUTH = 45

# When True, the plot is rendered once per entry in ROTATION_ANGLES and saved
# with a unique filename suffix.  Useful for publications or animations.
SAVE_MULTIPLE_ANGLES = True

# List of (elevation, azimuth) pairs to render when SAVE_MULTIPLE_ANGLES=True.
# The four entries below give views from each cardinal horizontal direction.
ROTATION_ANGLES = [(25, 45), (25, 135), (25, 225), (25, 315)]

# ── Data Loading ─────────────────────────────────────────────────────────────
# The C++ simulation saves results in NumPy's compressed archive format (.npz).
# Each key in the archive corresponds to a named array.
data = np.load('output/Out_3/ising_results.npz')

temperatures    = data['temperatures']     # Shape: (n_T,)
magnetic_fields = data['magnetic_fields']  # Shape: (n_h,)
magnetizations  = data['avg_magnetization']  # Shape: (n_h * n_T,)  – flattened

# ── Data Reshaping ───────────────────────────────────────────────────────────
# The C++ code stores magnetisation in row-major order: outer index = h, inner = T.
# Reshape to a proper 2-D array so we can feed it directly to matplotlib's
# surface/contour routines.
magnetizations_2d = magnetizations.reshape(len(magnetic_fields), len(temperatures))

# ── Meshgrid Construction ────────────────────────────────────────────────────
# np.meshgrid produces coordinate arrays that match the shape of magnetizations_2d.
# T_mesh[i, j] = temperatures[j],  H_mesh[i, j] = magnetic_fields[i]
T_mesh, H_mesh = np.meshgrid(temperatures, magnetic_fields)


# ── 3D Surface-Plot Helper ────────────────────────────────────────────────────
def save_3d_plot(elev, azim, output_suffix=''):
    """
    Create and save a 3D surface plot of average magnetisation vs (T, h).

    Parameters
    ----------
    elev          : float - camera elevation in degrees (0 = horizontal).
    azim          : float - camera azimuth in degrees (0 = looking along +x).
    output_suffix : str   - appended to the base filename before '.png', e.g. '_angle1'.
    """
    fig = plt.figure(figsize=(14, 9))
    ax  = fig.add_subplot(111, projection='3d')

    # Plot the magnetisation surface.  'viridis' maps low→high with a perceptually
    # uniform blue→yellow gradient.  alpha < 1 lets the grid lines show through.
    surf = ax.plot_surface(
        T_mesh, H_mesh, magnetizations_2d,
        cmap='viridis', alpha=0.85, edgecolor='none'
    )

    # Axis labels and title
    ax.set_xlabel('Temperature (T)',        fontsize=12, labelpad=10)
    ax.set_ylabel('Magnetic Field (h)',     fontsize=12, labelpad=10)
    ax.set_zlabel('Average Magnetization',  fontsize=12, labelpad=10)
    ax.set_title(
        '3D Ising Model: Magnetization vs Temperature and Magnetic Field',
        fontsize=14, pad=20
    )

    # Colorbar – maps surface colours back to magnetisation values.
    # shrink and pad adjust its size and distance from the axes.
    cbar = fig.colorbar(surf, ax=ax, label='Magnetization', shrink=0.6, pad=0.1)

    # Apply the requested viewing angle before saving
    ax.view_init(elev=elev, azim=azim)

    plt.tight_layout()

    # Build the output filename and write to disk at high DPI for print quality
    filename = f'output/Out_3/magnetization_3d_surface{output_suffix}.png'
    plt.savefig(filename, dpi=600, bbox_inches='tight')
    print(f"3D surface plot saved to {filename} (elev={elev}°, azim={azim}°)")
    plt.close()  # Release memory; avoids figure accumulation in long scripts


# ── Render and Save 3D Plot(s) ───────────────────────────────────────────────
if SAVE_MULTIPLE_ANGLES:
    # Iterate over all configured (elev, azim) pairs and save one file each
    print(f"\nSaving 3D plots from {len(ROTATION_ANGLES)} different angles:")
    for i, (elev, azim) in enumerate(ROTATION_ANGLES):
        save_3d_plot(elev, azim, f'_angle{i+1}')
else:
    # Save a single plot at the default viewing angle
    save_3d_plot(VIEW_ELEVATION, VIEW_AZIMUTH)


# ── 2D Contour Map ───────────────────────────────────────────────────────────
# Provides a top-down projection of the magnetisation surface.
# Filled contours (contourf) give a heat-map background; line contours (contour)
# add iso-lines that are labelled with clabel().
fig2, ax2 = plt.subplots(figsize=(11, 8))

# Use 20 evenly-spaced levels spanning the full data range for smooth colour gradients
levels = np.linspace(magnetizations_2d.min(), magnetizations_2d.max(), 20)

# Filled colour bands
contour = ax2.contourf(T_mesh, H_mesh, magnetizations_2d, levels=levels, cmap='viridis')

# Thin black iso-lines drawn on top of the filled contours
contour_lines = ax2.contour(
    T_mesh, H_mesh, magnetizations_2d,
    levels=levels, colors='black', alpha=0.2, linewidths=0.5
)

ax2.set_xlabel('Temperature (T)',          fontsize=12)
ax2.set_ylabel('Magnetic Field (h)',       fontsize=12)
ax2.set_title('Ising Model: Magnetization Contour Map', fontsize=14)

# Colorbar for the filled-contour plot
cbar2 = fig2.colorbar(contour, ax=ax2, label='Magnetization')

# Label the iso-lines with their magnetisation values (inline=True avoids overlap)
ax2.clabel(contour_lines, inline=True, fontsize=8)

plt.tight_layout()

# Save at moderate DPI – contour plots don't need the ultra-high res of surface plots
plt.savefig('output/Out_3/magnetization_contour.png', dpi=300, bbox_inches='tight')
print("Contour plot saved to output/Out_3/magnetization_contour.png")

plt.show()
