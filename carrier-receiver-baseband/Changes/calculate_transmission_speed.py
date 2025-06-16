import re
from datetime import datetime

def parse_line(line):
    # Example line: 00:11:05.269 | 0f ca 00 00 ... | -82 CRC error
    match = re.match(r"(\d{2}:\d{2}:\d{2}\.\d{3}) \| ([0-9a-fA-F ]+)\|", line)
    if match:
        timestamp_str = match.group(1)
        hex_data = match.group(2).strip()
        return timestamp_str, hex_data
    return None, None

def time_to_seconds(timestr):
    # Convert HH:MM:SS.mmm to seconds
    t = datetime.strptime(timestr, "%H:%M:%S.%f")
    return t.hour * 3600 + t.minute * 60 + t.second + t.microsecond / 1e6

def analyze_file(filename, is_hamming=False):
    timestamps = []
    total_bytes = []
    header_bytes = []
    payload_bytes = []
    
    with open(filename, "r") as f:
        for line in f:
            ts, hex_data = parse_line(line)
            if ts and hex_data:
                timestamps.append(ts)
                bytes_list = hex_data.split()
                # Assuming first 4 bytes are header
                header = bytes_list[:4]
                payload = bytes_list[4:]
                
                header_bytes.append(len(header))
                if is_hamming:
                    # Hamming encoded payload should be ~2x original size
                    payload_bytes.append(len(payload))
                else:
                    payload_bytes.append(len(payload))
                
                total_bytes.append(len(bytes_list))
    
    return {
        'timestamps': timestamps,
        'total_bytes': sum(total_bytes),
        'header_bytes': sum(header_bytes),
        'payload_bytes': sum(payload_bytes),
        'packet_count': len(timestamps)
    }

def calculate_speed(timestamps, packet_sizes):
    if len(timestamps) < 2:
        return 0, 0
    times = [time_to_seconds(ts) for ts in timestamps]
    total_bytes = sum(packet_sizes)
    duration = times[-1] - times[0]
    if duration <= 0:
        return 0, 0
    speed_bps = (total_bytes * 8) / duration
    speed_Bps = total_bytes / duration
    return speed_Bps, speed_bps

def verify_hamming_ratio(raw_file, encoded_file):
    raw_stats = analyze_file(raw_file)
    encoded_stats = analyze_file(encoded_file, is_hamming=True)
    
    expected_ratio = 2.0  # 7/4 ratio
    actual_ratio = encoded_stats['payload_bytes'] / raw_stats['payload_bytes']
    
    print(f"Expected Hamming payload ratio: {expected_ratio:.2f}")
    print(f"Actual payload ratio: {actual_ratio:.2f}")
    print(f"Header bytes ratio: {encoded_stats['header_bytes']/raw_stats['header_bytes']:.2f}")
    print(f"Packet count ratio: {encoded_stats['packet_count']/raw_stats['packet_count']:.2f}")

def main():
    input_file = "Realdata/Double_Baud/hamming_11b.txt"  # Change to "n2.txt" if needed
    stats = analyze_file(input_file)
    speed_Bps, speed_bps = calculate_speed(stats['timestamps'], [stats['total_bytes']])
    if len(stats['timestamps']) >= 2:
        start_time = time_to_seconds(stats['timestamps'][0])
        end_time = time_to_seconds(stats['timestamps'][-1])
        total_duration = end_time - start_time
        minutes = int(total_duration // 60)
        seconds = total_duration % 60
        duration_str = f"{minutes} min {seconds:.3f} sec" if minutes else f"{seconds:.3f} sec"
    else:
        duration_str = "N/A"
    report = (
        f"Analyzed file: {input_file}\n"
        f"Total packets: {stats['packet_count']}\n"
        f"Total bytes: {stats['total_bytes']}\n"
        f"Header bytes: {stats['header_bytes']}\n"
        f"Payload bytes: {stats['payload_bytes']}\n"
        f"Duration: {stats['timestamps'][-1]} - {stats['timestamps'][0]}\n"
        f"Total transmission time: {duration_str}\n"
        f"Average speed: {speed_Bps:.2f} Bytes/sec ({speed_bps:.2f} bits/sec)\n"
    )
    with open("transmission_speed_report.txt", "w") as f:
        f.write(report)
    print(report)

if __name__ == "__main__":
    main()