# Parity encoding for static payloads

# Define all possible static payloads you are sending
static_payloads = [
    bytes([0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76]),
    bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00]),
    bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB, 0xED, 0x0F])
]

def parity_encode_byte(byte):
    # Even parity: parity bit is 1 if number of 1s in byte is odd
    parity = (bin(byte).count('1')) % 2
    # Return original byte with parity as 9th bit (requires 2 bytes to store)
    return (byte << 1) | parity

def parity_encode_bytes(data):
    encoded = bytearray()
    for byte in data:
        encoded_byte = parity_encode_byte(byte)
        # Split 9-bit value into two bytes
        encoded.append(encoded_byte >> 8)
        encoded.append(encoded_byte & 0xFF)
    return bytes(encoded)

if __name__ == "__main__":
    c_arrays = []
    for idx, payload in enumerate(static_payloads):
        encoded = parity_encode_bytes(payload)
        print(f"Static payload {idx+1}:")
        print(f"  Original (hex): {payload.hex(' ')}")
        print(f"  Encoded  (hex): {encoded.hex(' ')}")
        print(f"  Encoded  (bin): {' '.join(f'{b:08b}' for b in encoded)}\n")
        # Prepare C array string
        c_array = ', '.join(f'0x{b:02x}' for b in encoded)
        c_arrays.append(f"uint8_t parity_encoded_payload_{idx+1}[{len(encoded)}] = {{ {c_array} }};")
    print("Copy the following arrays to your C code:\n")
    for arr in c_arrays:
        print(arr)