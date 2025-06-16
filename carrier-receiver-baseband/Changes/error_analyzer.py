import re
from collections import defaultdict
from typing import List, Dict, Tuple

class PacketAnalyzer:
    def __init__(self):
        self.error_patterns = defaultdict(int)
        self.packet_count = 0
        self.error_count = 0
        self.rssi_values = []
        self.previous_packet = None
        # Add known good packet pattern
        self.known_good_pattern = "0f"  # This should be your actual known good pattern
        self.file_type = None  # Add file type tracking
        
    def hex_to_binary(self, hex_str: str) -> str:
        """Convert hex string to binary string"""
        # Remove spaces and convert to binary
        hex_str = hex_str.replace(' ', '')
        binary = bin(int(hex_str, 16))[2:].zfill(len(hex_str) * 4)
        return binary
    
    def analyze_binary_differences(self, current_binary: str, previous_binary: str) -> Dict[str, any]:
        """Analyze differences between two binary strings"""
        if len(current_binary) != len(previous_binary):
            return {"error": "Different lengths"}
            
        differences = []
        burst_errors = []
        burst_start = -1
        consecutive_errors = 0
        
        for i, (curr, prev) in enumerate(zip(current_binary, previous_binary)):
            if curr != prev:
                differences.append(i)
                consecutive_errors += 1
                if burst_start == -1:
                    burst_start = i
            else:
                if consecutive_errors > 1:  # Consider 2 or more consecutive errors as a burst
                    burst_errors.append((burst_start, consecutive_errors))
                consecutive_errors = 0
                burst_start = -1
        
        # Check for final burst
        if consecutive_errors > 1:
            burst_errors.append((burst_start, consecutive_errors))
        
        return {
            "total_differences": len(differences),
            "difference_positions": differences,
            "burst_errors": burst_errors,
            "error_rate": len(differences) / len(current_binary)
        }
    
    def validate_packet(self, hex_data: str) -> Dict[str, any]:
        received_bytes = hex_data.split()
        expected_len = 32 if self.file_type == 'hamming' else 16
        is_valid_length = len(received_bytes) == expected_len
    
        return {
            "expected_length": expected_len,
            "actual_length": len(received_bytes)
        }
    
        # Check if packet starts with known good pattern
        is_valid_start = received_bytes[0] == self.known_good_pattern
        
        # Check packet length (should be 16 bytes for your system)
        is_valid_length = len(received_bytes) == 16
        
        # Check for expected patterns in specific positions
        validation_errors = []
        
        # Example validation rules (modify based on your protocol):
        # 1. First byte should be 0f
        if not is_valid_start:
            validation_errors.append("Invalid start byte")
        
        # 2. Check length
        if not is_valid_length:
            validation_errors.append(f"Invalid length: {len(received_bytes)} bytes")
        
        # 3. Check for expected patterns in specific positions
        # Add your specific validation rules here
        
        return {
            "is_valid": len(validation_errors) == 0,
            "validation_errors": validation_errors,
            "received_bytes": received_bytes,
            "expected_length": 16,
            "actual_length": len(received_bytes)
        }
    
    def decode_hamming(self, hex_data: str) -> str:
        """Decode Hamming(7,4) coded data and return corrected hex string"""
        bytes_list = hex_data.split()
        corrected_bytes = []
    
        for byte in bytes_list:
            # Convert to binary (7 bits)
            binary = bin(int(byte, 16))[2:].zfill(7)
            
            # Calculate parity bits
            p1 = int(binary[0]) ^ int(binary[1]) ^ int(binary[2]) ^ int(binary[4])
            p2 = int(binary[0]) ^ int(binary[1]) ^ int(binary[3]) ^ int(binary[5])
            p3 = int(binary[0]) ^ int(binary[2]) ^ int(binary[3]) ^ int(binary[6])
    
            # Error detection and correction
            error_pos = p1 + p2*2 + p3*4 - 1
            if error_pos >= 0:
                binary = list(binary)
                binary[error_pos] = '1' if binary[error_pos] == '0' else '0'
                binary = ''.join(binary)
    
            # Extract data bits (positions 3,5,6,7 -> indices 2,4,5,6)
            data_bits = binary[2] + binary[4] + binary[5] + binary[6]
            corrected_bytes.append(hex(int(data_bits, 2))[2:])
    
        return ' '.join(corrected_bytes)

    def analyze_packet(self, timestamp: str, hex_data: str, rssi: int, error_type: str) -> Dict[str, any]:
        """Analyze a single packet and return error information"""
        # Decode if Hamming file
        if self.file_type == 'hamming':
            hex_data = self.decode_hamming(hex_data)
        
        # Convert hex to binary
        binary_data = self.hex_to_binary(hex_data)
        
        # Analyze binary patterns
        binary_analysis = {
            "binary": binary_data,
            "ones_count": binary_data.count('1'),
            "zeros_count": binary_data.count('0'),
            "ones_ratio": binary_data.count('1') / len(binary_data)
        }
        
        # Compare with previous packet if available
        binary_diff = {}
        if self.previous_packet:
            binary_diff = self.analyze_binary_differences(binary_data, self.previous_packet)
        
        # Store current packet for next comparison
        self.previous_packet = binary_data
        
        # Analyze error patterns
        errors = {
            'crc_error': 'CRC error' in error_type,
            'bit_errors': self._detect_bit_errors(hex_data),
            'timing_errors': self._check_timing_consistency(timestamp),
            'signal_quality': self._analyze_signal_quality(rssi),
            'binary_analysis': binary_analysis,
            'binary_diff': binary_diff
        }
        
        return errors
    
    def _detect_bit_errors(self, hex_data: str) -> List[Tuple[int, int]]:
        """Detect potential bit errors in the packet"""
        # Convert hex to binary
        binary_data = bin(int(hex_data.replace(' ', ''), 16))[2:].zfill(len(hex_data.replace(' ', '')) * 4)
        
        # Look for patterns that might indicate bit errors
        bit_errors = []
        for i in range(0, len(binary_data), 8):
            byte = binary_data[i:i+8]
            if byte.count('1') % 2 != 0:  # Simple parity check
                bit_errors.append((i//8, int(byte, 2)))
        
        return bit_errors
    
    def _check_timing_consistency(self, timestamp: str) -> bool:
        """Check if the packet timing is consistent"""
        # This would need to be implemented based on your timing requirements
        return True
    
    def _analyze_signal_quality(self, rssi: int) -> str:
        """Analyze signal quality based on RSSI"""
        if rssi > -60:
            return "Excellent"
        elif rssi > -70:
            return "Good"
        elif rssi > -80:
            return "Fair"
        else:
            return "Poor"

def parse_results_file(file_path: str, file_type: str = 'static') -> List[Dict[str, any]]:
    """Parse the results file and return analyzed packets"""
    analyzer = PacketAnalyzer()
    analyzer.file_type = file_type
    results = []
    
    try:
        with open(file_path, 'r') as file:
            for line in file:
                match = re.match(r'(\d{2}:\d{2}:\d{2}\.\d{3}) \| ([0-9a-f ]+) \| (-?\d+) (.*)', line.strip())
                if match:
                    timestamp, hex_data, rssi, error_type = match.groups()
                    rssi = int(rssi)
                    
                    # Analyze the packet
                    errors = analyzer.analyze_packet(timestamp, hex_data, rssi, error_type)
                    results.append({
                        'timestamp': timestamp,
                        'hex_data': hex_data,
                        'rssi': rssi,
                        'error_type': error_type,
                        'detailed_errors': errors
                    })
                    
                    analyzer.packet_count += 1
                    if 'CRC error' in error_type:
                        analyzer.error_count += 1
                    analyzer.rssi_values.append(rssi)
    
    except FileNotFoundError:
        print(f"Error: File '{file_path}' not found.")
        return []
    
    return results

def identify_error_type(binary_analysis, binary_diff):
    """Identify the likely type of errors based on the analysis"""
    error_types = []
    
    # Check for burst errors
    if binary_diff and 'burst_errors' in binary_diff and binary_diff['burst_errors']:
        error_types.append(f"Burst Errors detected: {len(binary_diff['burst_errors'])} bursts")
        for start, length in binary_diff['burst_errors']:
            error_types.append(f"  - Burst at bit {start}, length {length}")
    
    # Check for random bit errors
    if binary_diff and 'difference_positions' in binary_diff:
        isolated_errors = sum(1 for i, pos in enumerate(binary_diff['difference_positions'])
                            if i == 0 or pos - binary_diff['difference_positions'][i-1] > 1)
        if isolated_errors > 0:
            error_types.append(f"Random Bit Errors: {isolated_errors} isolated errors")
    
    # Check bit distribution
    if binary_analysis:
        ones_ratio = binary_analysis['ones_ratio']
        if not (0.45 <= ones_ratio <= 0.55):  # Expected roughly equal 1s and 0s
            error_types.append(f"Biased Bit Distribution: {ones_ratio:.2%} ones")
    
    return error_types

def print_analysis(results: List[Dict[str, any]]):
    """Print final BER and packet loss rate statistics"""
    if not results:
        print("No results to analyze.")
        return
    
    # Calculate error statistics
    total_packets = len(results)
    error_packets = sum(1 for result in results if 'CRC error' in result['error_type'])
    total_bits = sum(len(result['hex_data'].replace(' ', '')) * 4 for result in results)  # Each hex digit = 4 bits
    error_bits = sum(len(result['detailed_errors'].get('binary_diff', {}).get('difference_positions', [])) 
                   for result in results if 'binary_diff' in result['detailed_errors'])
    
    # Calculate percentages
    packet_loss_percentage = (error_packets / total_packets) * 100 if total_packets > 0 else 0
    bit_error_percentage = (error_bits / total_bits) * 100 if total_bits > 0 else 0
    
    print("\n=== Final Statistics ===")
    print(f"Bit Error Rate (BER): {bit_error_percentage:.2f}%")
   

if __name__ == "__main__":
    # Example usage for both file types
    static_results = parse_results_file("Realdata/static_1a.txt", file_type='static')
    hamming_results = parse_results_file("Realdata/Double_Baud/hamming_44b.txt", file_type='hamming')
    
    print("\nStatic File Analysis:")
    print_analysis(static_results)
    
    print("\nHamming File Analysis:")
    print_analysis(hamming_results)