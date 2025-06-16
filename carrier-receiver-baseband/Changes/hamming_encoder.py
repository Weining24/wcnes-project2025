import numpy as np

# Define all possible static payloads you are sending
static_payloads = [
    bytes([0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76]),
    bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00]),
    bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB, 0xED, 0x0F])
]

def hamming74_encode_nibble(nibble):
    # Hamming(7,4) generator matrix (G)
    G = np.array([
        [1,0,0,0,0,1,1],
        [0,1,0,0,1,0,1],
        [0,0,1,0,1,1,0],
        [0,0,0,1,1,1,1]
    ])
    data_bits = np.array([(nibble >> i) & 1 for i in range(3,-1,-1)])  # 4 bits
    codeword = np.dot(data_bits, G) % 2
    codeword_int = 0
    for bit in codeword:
        codeword_int = (codeword_int << 1) | int(bit)
    return codeword_int

def hamming74_encode_bytes(data):
    encoded = []
    for byte in data:
        high_nibble = (byte >> 4) & 0xF
        low_nibble = byte & 0xF
        # Problem: Storing 7-bit codes as full bytes without bit packing
        encoded.append(hamming74_encode_nibble(high_nibble))  # 1 byte
        encoded.append(hamming74_encode_nibble(low_nibble))   # 1 byte
    return bytes(encoded)

if __name__ == "__main__":
    c_arrays = []
    for idx, payload in enumerate(static_payloads):
        encoded = hamming74_encode_bytes(payload)
        print(f"Static payload {idx+1}:")
        print(f"  Original (hex): {payload.hex(' ')}")
        print(f"  Encoded  (hex): {encoded.hex(' ')}")
        print(f"  Encoded  (bin): {' '.join(f'{b:07b}' for b in encoded)}\n")
        # Prepare C array string
        c_array = ', '.join(f'0x{b:02x}' for b in encoded)
        c_arrays.append(f"uint8_t encoded_payload_{idx+1}[{len(encoded)}] = {{ {c_array} }};")
    print("Copy the following arrays to your C code:\n")
    for arr in c_arrays:
        print(arr)