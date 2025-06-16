import matplotlib.pyplot as plt
import numpy as np

def convert_comma_strings_to_float(data_list):
    return [float(item.replace(',', '.')) for item in data_list]

def plot_combined(baseline_perf, hamming_perf, baseline_time, hamming_time, labels):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # Prepare x-axis positions
    x = np.arange(len(labels))  # Positions of the groups
    width = 0.2  # Set the bar width

    # Convert all string data to floats
    baseline_perf = convert_comma_strings_to_float(baseline_perf)
    hamming_perf = convert_comma_strings_to_float(hamming_perf)
    baseline_time = convert_comma_strings_to_float(baseline_time)
    hamming_time = convert_comma_strings_to_float(hamming_time)

    # Plot performance metrics
    bars1 = ax1.bar(x - width/2, baseline_perf, width, color='blue', label='Baseline')
    bars2 = ax1.bar(x + width/2, hamming_perf, width, color='red', label='Hamming')
    ax1.set_title('BER vs Distance (100000 Baud rate)')
    ax1.set_ylabel('BER')
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels)
    ax1.bar_label(bars1, fmt='%.2f')  # Label for baseline bars
    ax1.bar_label(bars2, fmt='%.2f')  # Label for hamming bars
    ax1.legend(loc='upper left', bbox_to_anchor=(1, 0.5))
    ax1.set_ylim(0, 50)

    # Plot time durations
    bars3 = ax2.bar(x - width/2, baseline_time, width, color='blue', label='Baseline')
    bars4 = ax2.bar(x + width/2, hamming_time, width, color='red', label='Hamming')
    ax2.set_title('BER vs Distance (200000 Baud rate)')
    ax2.set_ylabel('BER')
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels)
    ax2.bar_label(bars3, fmt='%.2f')  # Label for baseline time bars
    ax2.bar_label(bars4, fmt='%.2f')  # Label for hamming time bars
    ax2.set_ylim(0, 50)

    plt.tight_layout()
    plt.savefig('combined_comparison.png')
    plt.show()

if __name__ == "__main__":
    baseline_perf = ['43,06', '43,2', '43,23']
    hamming_perf = ['22,73', '22,92', '21,3']
    baseline_time = ['42,3', '42,78', '42,57']
    hamming_time = ['22,63', '22,9', '21.12']

    # Custom names for each model
    labels = ['1 cm', '20 cm', '40 cm']
    
    plot_combined(baseline_perf, hamming_perf, baseline_time, hamming_time, labels)
