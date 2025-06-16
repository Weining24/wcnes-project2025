import matplotlib.pyplot as plt
import numpy as np

def convert_min_sec_to_float(time_list):
    """Convert 'X min YY' strings to float (total minutes)."""
    converted = []
    for time_str in time_list:
        parts = time_str.split()
        minutes = float(parts[0])  # Extract minutes
        seconds = float(parts[2])  # Extract seconds
        total_minutes = minutes + (seconds / 60.0)  # Convert to fractional minutes
        converted.append(total_minutes)
    return converted

def plot_combined(baseline_perf, hamming_perf, baseline_time, hamming_time, labels):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # Prepare x-axis positions
    x = np.arange(len(labels))  # Positions of the groups
    width = 0.2  # Set the bar width

    # Store original labels for display
    baseline_perf_labels = baseline_perf
    hamming_perf_labels = hamming_perf
    baseline_time_labels = baseline_time
    hamming_time_labels = hamming_time

    # Convert time strings to float (for bar heights)
    baseline_perf = convert_min_sec_to_float(baseline_perf)
    hamming_perf = convert_min_sec_to_float(hamming_perf)
    baseline_time = convert_min_sec_to_float(baseline_time)
    hamming_time = convert_min_sec_to_float(hamming_time)

    # Plot performance metrics
    bars1 = ax1.bar(x - width/2, baseline_perf, width, color='green', label='Baseline')
    bars2 = ax1.bar(x + width/2, hamming_perf, width, color='yellow', label='Hamming')
    ax1.set_title('Speed vs Distance (100000 Baud rate)')
    ax1.set_ylabel('Time (minutes)')  # Updated y-label
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels)
    ax1.bar_label(bars1, labels=baseline_perf_labels, padding=3)  # Use original strings
    ax1.bar_label(bars2, labels=hamming_perf_labels, padding=3)  # Use original strings
    ax1.legend(loc='upper left', bbox_to_anchor=(1, 0.5))
    ax1.set_ylim(0, 10)  # Fix y-axis from 0 to 10

    # Plot time durations
    bars3 = ax2.bar(x - width/2, baseline_time, width, color='green', label='Baseline')
    bars4 = ax2.bar(x + width/2, hamming_time, width, color='yellow', label='Hamming')
    ax2.set_title('Speed vs Distance (200000 Baud rate)')
    ax2.set_ylabel('Time (minutes)')  # Updated y-label
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels)
    ax2.bar_label(bars3, labels=baseline_time_labels, padding=2)  # Use original strings
    ax2.bar_label(bars4, labels=hamming_time_labels, padding=2)  # Use original strings
    ax2.set_ylim(0, 10)  # Fix y-axis from 0 to 10

    plt.tight_layout()
    plt.savefig('combined_comparison.png')
    plt.show()

if __name__ == "__main__":
    baseline_perf = ['4 min 26', '4 min 27', '4 min 30']
    hamming_perf = ['4 min 40', '4 min 49', '9 min 30']
    baseline_time = ['2 min 31', '2 min 26', '2 min 41']
    hamming_time = ['2 min 44', '2 min 22', '4 min 22']

    # Custom names for each model
    labels = ['1 cm', '20 cm', '40 cm']
    
    plot_combined(baseline_perf, hamming_perf, baseline_time, hamming_time, labels)