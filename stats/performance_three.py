import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D

def convert_comma_string_to_float(data_list):
    """Convert European decimal strings to floats"""
    return [float(item.replace(',', '.')) for item in data_list]

# Original time strings (unchanged)
baseline_time_100k_str = ['4 min 26', '4 min 27', '4 min 30']
hamming_time_100k_str = ['4 min 40', '4 min 49', '9 min 30'] 
baseline_time_200k_str = ['2 min 31', '2 min 26', '2 min 41']
hamming_time_200k_str = ['2 min 44', '2 min 22', '4 min 22']

def convert_min_sec_to_float(time_list):
    """Convert 'X min YY' format to total minutes (for plotting only)"""
    converted = []
    for time_str in time_list:
        parts = time_str.split()
        minutes = float(parts[0])
        seconds = float(parts[2])
        converted.append(minutes + seconds/60)
    return converted

# Input data
distances = [1, 20, 40]  # Numeric values in cm

# BER data (converted to float)
baseline_ber_100k = convert_comma_string_to_float(['43,06', '43,2', '43,23'])
hamming_ber_100k = convert_comma_string_to_float(['22,73', '22,92', '21,3'])
baseline_ber_200k = convert_comma_string_to_float(['42,3', '42,78', '42,57'])
hamming_ber_200k = convert_comma_string_to_float(['22,63', '22,9', '21.12'])

# Time data (converted to float for plotting, but keep original strings for labels)
baseline_time_100k = convert_min_sec_to_float(baseline_time_100k_str)
hamming_time_100k = convert_min_sec_to_float(hamming_time_100k_str)
baseline_time_200k = convert_min_sec_to_float(baseline_time_200k_str)
hamming_time_200k = convert_min_sec_to_float(hamming_time_200k_str)

# Create figure
fig = plt.figure(figsize=(16, 12))
ax = fig.add_subplot(111, projection='3d')

# Plot all data
ax.plot(distances, baseline_ber_100k, baseline_time_100k,
        'b-o', linewidth=2, markersize=10, label='Baseline (100k)')
ax.plot(distances, hamming_ber_100k, hamming_time_100k,
        'r-o', linewidth=2, markersize=10, label='Hamming (100k)')
ax.plot(distances, baseline_ber_200k, baseline_time_200k,
        'b--s', linewidth=2, markersize=10, label='Baseline (200k)')
ax.plot(distances, hamming_ber_200k, hamming_time_200k,
        'r--s', linewidth=2, markersize=10, label='Hamming (200k)')

# Function to add labels using original time strings
def add_labels(x, y, time_strs, color, offset=0.5):
    for i, (xi, yi, tstr) in enumerate(zip(x, y, time_strs)):
        # Alternate label positions to avoid overlap
        z_pos = convert_min_sec_to_float([tstr])[0]  # Get z position from time string
        label_pos = (xi, yi + offset*(i%2)*2, z_pos + offset*((i+1)%2))
        ax.text(*label_pos, f'({xi}cm, {yi:.1f}%, {tstr})', 
               color=color, fontsize=9, zorder=10)

# Add labels using original time strings
add_labels(distances, baseline_ber_100k, baseline_time_100k_str, 'blue')
add_labels(distances, hamming_ber_100k, hamming_time_100k_str, 'red')
add_labels(distances, baseline_ber_200k, baseline_time_200k_str, 'blue', offset=0.7)
add_labels(distances, hamming_ber_200k, hamming_time_200k_str, 'red', offset=0.7)

# Axis labels and title
ax.set_xlabel('Distance (cm)', fontsize=12, labelpad=15)
ax.set_ylabel('BER (%)', fontsize=12, labelpad=15)
ax.set_zlabel('Time', fontsize=12, labelpad=15)
ax.set_title('3D Analysis: BER vs Distance vs Transmission Time', 
            fontsize=16, pad=25)

# Legend and grid
ax.legend(fontsize=10, bbox_to_anchor=(1.1, 0.9), loc='upper left')
ax.grid(True)

# Adjust view and layout
ax.view_init(elev=25, azim=-45)
ax.dist = 10  # Slightly pull back the camera
plt.tight_layout()

# Save and show
plt.savefig('3d_ber_analysis_original_times.png', dpi=300, bbox_inches='tight')
plt.show()