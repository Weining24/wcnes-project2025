import re

# Define all possible static payloads you are sending
static_payloads = [
    bytes([0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76]),
    bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00]),
    bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB, 0xED, 0x0F])
]

def extract_hex_bytes(line):
    match = re.search(r'\|\s*([0-9a-fA-F ]+)\s*\|', line)
    if match:
        hex_str = match.group(1)
        return bytes.fromhex(hex_str)
    return b''

def bytes_to_binstr(b):
    return ' '.join(f'{byte:08b}' for byte in b)

def parse_payload(payload_string):
    """Parse hex payload into list of integers"""
    return [int(x, base=16) for x in payload_string.split()]

def compute_bit_errors(received, expected):
    """More comprehensive bit error calculation"""
    errors = 0
    for x, y in zip(received, expected):
        errors += bin(x ^ y).count('1')
    # Add errors for length mismatches
    errors += 8 * abs(len(received) - len(expected))
    return errors

def compute_ber(packets, expected_payloads):
    """Improved BER calculation"""
    total_errors = 0
    total_bits = 0
    for payload in packets:
        min_errors = float('inf')
        for expected in expected_payloads:
            errors = compute_bit_errors(payload, expected)
            min_errors = min(min_errors, errors)
        total_errors += min_errors
        total_bits += len(payload) * 8
    return total_errors / total_bits if total_bits > 0 else 0

def find_burst_errors(a: bytes, b: bytes) -> list:
    """Return a list of (start, length) for each burst of consecutive bit errors between a and b."""
    bursts = []
    burst_start = None
    burst_length = 0
    for i, (x, y) in enumerate(zip(a, b)):
        diff = x ^ y
        for bit in range(8):
            if diff & (1 << (7 - bit)):
                if burst_start is None:
                    burst_start = i * 8 + bit
                    burst_length = 1
                else:
                    burst_length += 1
            else:
                if burst_length > 1:
                    bursts.append((burst_start, burst_length))
                burst_start = None
                burst_length = 0
    if burst_length > 1:
        bursts.append((burst_start, burst_length))
    return bursts

total = 0
match_count = 0
no_match_count = 0
total_bits = 0
total_bit_errors = 0
single_bit_error_count = 0
parity_bit_error_count = 0
other_error_count = 0

packets = []
with open("static_111.txt", "r") as f:
    for idx, line in enumerate(f):
        packet = extract_hex_bytes(line)
        if packet and len(packet) > 4:
            payload = packet[4:]  # Skip the first 4 bytes (header)
            total += 1
            match_found = False
            for expected in static_payloads:
                expected_trunc = expected[:len(payload)]
                if payload == expected_trunc:
                    match_found = True
                    match_count += 1
                    break
            if not match_found:
                no_match_count += 1
                min_errors = None
                # Compare only the payload (skip first 4 header bytes)
                payload_data = packet[4:]  # This is the actual data portion
                for expected in static_payloads:
                    expected_trunc = expected[:len(payload_data)]
                    errors = compute_bit_errors(payload_data, expected_trunc)
                    if min_errors is None or errors < min_errors:
                        min_errors = errors
                total_bit_errors += min_errors if min_errors is not None else 0
                total_bits += len(payload) * 8  # Use actual payload length
                # Classify error type
                if min_errors == 1:
                    single_bit_error_count += 1
                elif min_errors is not None and min_errors > 1:
                    if min_errors % 2 == 0:
                        parity_bit_error_count += 1
                    else:
                        other_error_count += 1
            else:
                total_bits += len(payload) * 8  # All bits correct for matched packets

if total > 0:
    match_percent = (match_count / total) * 100
    no_match_percent = (no_match_count / total) * 100
    ber = (total_bit_errors / total_bits) if total_bits > 0 else 0
    single_bit_error_percent = (single_bit_error_count / total) * 100
    parity_bit_error_percent = (parity_bit_error_count / total) * 100
    other_error_percent = (other_error_count / total) * 100
    
    print("\nSummary Statistics:")
    print(f"Total packets: {total}")
    print(f"Matches: {match_count} ({match_percent:.2f}%)")
    print(f"No matches: {no_match_count} ({no_match_percent:.2f}%)")
    print(f"\nBit Error Analysis:")
    print(f"Bit Error Rate (BER): {ber*100:.2f}%")
    print(f"Single-bit errors: {single_bit_error_count} ({single_bit_error_percent:.2f}%)")
    print(f"Parity (even # of bits) errors: {parity_bit_error_count} ({parity_bit_error_percent:.2f}%)")
    print(f"Other errors: {other_error_count} ({other_error_percent:.2f}%)")
else:
    print("No packets found.")