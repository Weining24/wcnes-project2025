import re

# Same static payloads as in the encoder
static_payloads = [
    bytes([0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76]),
    bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00]),
    bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB, 0xED, 0x0F])
]

def parity_decode_byte(encoded_pair):
    """Decode a byte+parity pair"""
    if len(encoded_pair) != 2:
        return None, False
        
    byte = encoded_pair[0]
    parity_bit = encoded_pair[1]
    
    # Calculate expected parity
    expected_parity = bin(byte).count('1') % 2
    
    # Check for errors
    if parity_bit != expected_parity:
        return byte, False  # Data with parity error
    return byte, True  # Correct data

def parity_decode_bytes(encoded_data):
    """Decode sequence of byte+parity pairs"""
    decoded = []
    errors = []
    
    for i in range(0, len(encoded_data)-1, 2):
        byte, is_correct = parity_decode_byte(encoded_data[i:i+2])
        decoded.append(byte)
        errors.append(not is_correct)
        
    return bytes(decoded), errors

def extract_hex_bytes(line):
    """Extract hex bytes from a log line"""
    match = re.search(r'\|\s*([0-9a-fA-F ]+)\s*\|', line)
    if match:
        hex_str = match.group(1)
        return bytes.fromhex(hex_str)
    return b''

def bytes_to_binstr(b):
    """Convert bytes to binary string representation"""
    return ' '.join(f'{byte:08b}' for byte in b)

def count_bit_errors(a: bytes, b: bytes) -> int:
    """Count differing bits between two byte arrays"""
    return sum(bin(x ^ y).count('1') for x, y in zip(a, b))

def find_burst_errors(a: bytes, b: bytes) -> list:
    """Find bursts of consecutive bit errors"""
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

# Statistics counters
total = 0
match_count = 0
no_match_count = 0
total_bits = 0
total_bit_errors = 0
single_bit_error_count = 0
multi_bit_error_count = 0
burst_error_count = 0

# Example usage with a log file
with open("Data/parity_2.txt", "r") as f:
    for idx, line in enumerate(f):
        packet = extract_hex_bytes(line)
        if packet and len(packet) > 4:
            encoded_payload = packet[4:]  # Skip header
            decoded_payload, errors = parity_decode_bytes(encoded_payload)
            total += 1
            
            match_found = False
            for expected in static_payloads:
                expected_trunc = expected[:len(decoded_payload)]
                if decoded_payload == expected_trunc:
                    match_found = True
                    match_count += 1
                    print(f"Packet {idx}: MATCHED static payload")
                    print(f"  Decoded (hex):    {decoded_payload.hex(' ')}")
                    print(f"  Expected (hex):   {expected_trunc.hex(' ')}")
                    print(f"  Decoded (bin):    {bytes_to_binstr(decoded_payload)}\n")
                    break
            
            if not match_found:
                no_match_count += 1
                min_errors = None
                min_bursts = None
                for expected in static_payloads:
                    expected_trunc = expected[:len(decoded_payload)]
                    errors = count_bit_errors(decoded_payload, expected_trunc)
                    bursts = find_burst_errors(decoded_payload, expected_trunc)
                    if min_errors is None or errors < min_errors:
                        min_errors = errors
                        min_bursts = bursts
                
                total_bit_errors += min_errors if min_errors is not None else 0
                total_bits += len(decoded_payload) * 8
                
                # Classify error type
                if min_errors == 1:
                    single_bit_error_count += 1
                elif min_errors > 1:
                    multi_bit_error_count += 1
                
                if min_bursts and len(min_bursts) > 0:
                    burst_error_count += 1
                
                print(f"Packet {idx}: NO MATCH")
                print(f"  Decoded (hex):    {decoded_payload.hex(' ')}")
                print(f"  Decoded (bin):    {bytes_to_binstr(decoded_payload)}")
                print(f"  Error count:      {min_errors}")
                if min_bursts:
                    print(f"  Burst errors:     {min_bursts}")
                print()

if total > 0:
    match_percent = (match_count / total) * 100
    no_match_percent = (no_match_count / total) * 100
    ber = (total_bit_errors / total_bits) if total_bits > 0 else 0
    single_bit_error_percent = (single_bit_error_count / total) * 100
    multi_bit_error_percent = (multi_bit_error_count / total) * 100
    burst_error_percent = (burst_error_count / total) * 100
    
    print("\nSummary Statistics:")
    print(f"Total packets: {total}")
    print(f"Matches: {match_count} ({match_percent:.2f}%)")
    print(f"No matches: {no_match_count} ({no_match_percent:.2f}%)")
    print(f"Bit Error Rate (BER): {ber*100:.2f}%")
    print(f"Single-bit errors: {single_bit_error_count} ({single_bit_error_percent:.2f}%)")
    print(f"Multi-bit errors: {multi_bit_error_count} ({multi_bit_error_percent:.2f}%)")
    print(f"Burst errors: {burst_error_count} ({burst_error_percent:.2f}%)")
else:
    print("No packets found.")