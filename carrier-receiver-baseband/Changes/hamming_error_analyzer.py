import sys
from typing import List, Dict

class HammingErrorAnalyzer:
    def __init__(self):
        self.expected_payloads = [
            bytes([0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF]),
            bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC]),
            bytes([0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x21, 0x43, 0x65, 0x87])
        ]

    def hamming74_decode(self, codeword: int) -> int:
        """Decode a 7-bit Hamming codeword to 4-bit data"""
        H = [
            [1,0,1,0,1,0,1],
            [0,1,1,0,0,1,1],
            [0,0,0,1,1,1,1]
        ]
        bits = [(codeword >> (6-i)) & 1 for i in range(7)]
        syndrome = [sum([bits[j]*H[i][j] for j in range(7)]) % 2 for i in range(3)]
        syndrome_val = (syndrome[0]<<2) | (syndrome[1]<<1) | syndrome[2]
        if syndrome_val != 0:
            error_pos = syndrome_val - 1
            if 0 <= error_pos < 7:
                bits[error_pos] ^= 1
        return (bits[0]<<3) | (bits[1]<<2) | (bits[2]<<1) | bits[3]

    def hamming74_decode_bytes(self, encoded: bytes) -> bytes:
        """Decode Hamming-encoded bytes"""
        decoded = []
        for i in range(0, len(encoded)-1, 2):
            high_nibble = self.hamming74_decode(encoded[i])
            low_nibble = self.hamming74_decode(encoded[i+1])
            decoded.append((high_nibble << 4) | low_nibble)
        return bytes(decoded)

    def analyze_file(self, filename: str) -> Dict[str, float]:
        """Analyze a file containing Hamming-encoded data"""
        total_bits = 0
        error_bits = 0
        total_packets = 0
        error_packets = 0
        
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                    
                total_packets += 1
                hex_data = line.split()[0]  # Assuming first field is hex data
                encoded_bytes = bytes.fromhex(hex_data)
                
                # Skip header if present (first 4 bytes)
                payload = encoded_bytes[4:] if len(encoded_bytes) > 4 else encoded_bytes
                decoded = self.hamming74_decode_bytes(payload)
                
                # Find closest matching expected payload
                min_errors = float('inf')
                for expected in self.expected_payloads:
                    expected_trunc = expected[:len(decoded)]
                    errors = sum(bin(x ^ y).count('1') for x, y in zip(decoded, expected_trunc))
                    min_errors = min(min_errors, errors)
                
                error_bits += min_errors
                total_bits += len(decoded) * 8
                if min_errors > 0:
                    error_packets += 1
        
        return {
            'total_packets': total_packets,
            'error_packets': error_packets,
            'packet_error_rate': error_packets / total_packets if total_packets > 0 else 0,
            'total_bits': total_bits,
            'error_bits': error_bits,
            'bit_error_rate': error_bits / total_bits if total_bits > 0 else 0
        }

def main():
    if len(sys.argv) != 2:
        print("Usage: python hamming_error_analyzer.py <input_file>")
        return
    
    analyzer = HammingErrorAnalyzer()
    results = analyzer.analyze_file(sys.argv[1])
    
    print("\n=== Hamming Code Analysis Results ===")
    print(f"Total Packets: {results['total_packets']}")
    print(f"Error Packets: {results['error_packets']}")
    print(f"Packet Error Rate: {results['packet_error_rate']*100:.2f}%")
    print(f"\nTotal Bits: {results['total_bits']}")
    print(f"Error Bits: {results['error_bits']}")
    print(f"Bit Error Rate: {results['bit_error_rate']*100:.2f}%")

if __name__ == "__main__":
    main()