import re

# The original static payloads (before Hamming encoding)
static_payloads = [
    bytes([0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76]),
    bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00]),
    bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB, 0xED, 0x0F])
]

# Hamming(7,4) decode table for all 128 possible codewords
def hamming74_decode(codeword):
    # Syndrome table for Hamming(7,4)
    syndrome_table = [
        0b0000000, 0b0000001, 0b0000010, 0b0000100, 0b0001000, 0b0010000, 0b0100000
    ]
    # Parity-check matrix
    H = [
        [1,0,1,0,1,0,1],
        [0,1,1,0,0,1,1],
        [0,0,0,1,1,1,1]
    ]
    # Convert codeword to bits
    bits = [(codeword >> (6-i)) & 1 for i in range(7)]
    # Calculate syndrome
    syndrome = [sum([bits[j]*H[i][j] for j in range(7)]) % 2 for i in range(3)]
    syndrome_val = (syndrome[0]<<2) | (syndrome[1]<<1) | syndrome[2]
    # If syndrome is not zero, correct the bit
    if syndrome_val != 0:
        error_pos = syndrome_val - 1  # Correct error position calculation (0-based)
        if 0 <= error_pos < 7:
            bits[error_pos] ^= 1
    # Extract data bits (positions 0,1,2,3)
    data = (bits[0]<<3) | (bits[1]<<2) | (bits[2]<<1) | bits[3]
    return data

def hamming74_decode_bytes(encoded):
    decoded = []
    # Each original byte is encoded as two 7-bit codewords (2 bytes)
    for i in range(0, len(encoded)-1, 2):
        high_nibble = hamming74_decode(encoded[i])
        low_nibble = hamming74_decode(encoded[i+1])
        decoded.append((high_nibble << 4) | low_nibble)
    return bytes(decoded)

def extract_hex_bytes(line):
    match = re.search(r'\|\s*([0-9a-fA-F ]+)\s*\|', line)
    if match:
        hex_str = match.group(1)
        return bytes.fromhex(hex_str)
    return b''

def bytes_to_binstr(b):
    return ' '.join(f'{byte:08b}' for byte in b)

def count_bit_errors(a: bytes, b: bytes) -> int:
    # Compare two byte arrays and count differing bits
    return sum(bin(x ^ y).count('1') for x, y in zip(a, b))

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
burst_error_count = 0

with open("hamming_1.txt", "r") as f:
    for idx, line in enumerate(f):
        packet = extract_hex_bytes(line)
        if packet and len(packet) > 4:
            encoded_payload = packet[4:]  # Skip the first 4 bytes (header)
            decoded_payload = hamming74_decode_bytes(encoded_payload)
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
                elif min_errors is not None and min_errors % 2 == 0:
                    parity_bit_error_count += 1
                else:
                    other_error_count += 1
                if min_bursts and len(min_bursts) > 0:
                    burst_error_count += 1
                print(f"Packet {idx}: NO MATCH")
                print(f"  Decoded (hex):    {decoded_payload.hex(' ')}")
                print(f"  Decoded (bin):    {bytes_to_binstr(decoded_payload)}")
                if min_bursts:
                    print(f"  Burst errors: {min_bursts}")
                else:
                    print(f"  Burst errors: None")
                print()
            else:
                total_bits += len(decoded_payload) * 8  # All bits correct for matched packets

if total > 0:
    match_percent = (match_count / total) * 100
    no_match_percent = (no_match_count / total) * 100
    ber = (total_bit_errors / total_bits) if total_bits > 0 else 0
    single_bit_error_percent = (single_bit_error_count / total) * 100
    parity_bit_error_percent = (parity_bit_error_count / total) * 100
    other_error_percent = (other_error_count / total) * 100
    burst_error_percent = (burst_error_count / total) * 100
    print(f"Total packets: {total}")
    print(f"Matches: {match_count} ({match_percent:.2f}%)")
    print(f"No matches: {no_match_count} ({no_match_percent:.2f}%)")
    print(f"Bit Error Rate (BER): ({ber*100:.2f}%)")
    print(f"Single-bit errors: {single_bit_error_count} ({single_bit_error_percent:.2f}%)")
    print(f"Parity (even # of bits) errors: {parity_bit_error_count} ({parity_bit_error_percent:.2f}%)")
    print(f"Other errors: {other_error_count} ({other_error_percent:.2f}%)")
    print(f"Burst errors: {burst_error_count} ({burst_error_percent:.2f}%)")
else:
    print("No packets found.")