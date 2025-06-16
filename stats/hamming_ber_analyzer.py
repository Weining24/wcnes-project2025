#!/usr/bin/env python3
#
# Copyright 2024
#
# This file implements Hamming code decoding and BER calculation with detailed error analysis

import numpy as np
import pandas as pd
from io import StringIO
import matplotlib.pyplot as plt
from typing import List, Tuple, Dict

# Hamming(7,4) decode table for all 128 possible codewords
def hamming74_decode(codeword: int) -> int:
    """Decode a 7-bit Hamming codeword to 4-bit data"""
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

def hamming74_decode_bytes(encoded: bytes) -> bytes:
    """Decode a sequence of Hamming-encoded bytes"""
    decoded = []
    # Each original byte is encoded as two 7-bit codewords (2 bytes)
    for i in range(0, len(encoded)-1, 2):
        high_nibble = hamming74_decode(encoded[i])
        low_nibble = hamming74_decode(encoded[i+1])
        decoded.append((high_nibble << 4) | low_nibble)
    return bytes(decoded)

def count_bit_errors(a: bytes, b: bytes) -> int:
    """Count the number of bit errors between two byte arrays"""
    return sum(bin(x ^ y).count('1') for x, y in zip(a, b))

def find_burst_errors(a: bytes, b: bytes) -> List[Tuple[int, int]]:
    """Find bursts of consecutive bit errors between two byte arrays"""
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

def parse_payload(payload_string: str) -> bytes:
    """Parse hex string payload into bytes"""
    return bytes([int(x, 16) for x in payload_string.split()])

def compute_ber_with_hamming(df: pd.DataFrame, PACKET_LEN: int = 32) -> Dict:
    """Compute BER with Hamming code decoding and detailed error analysis"""
    if len(df) == 0:
        print("Warning: Empty dataframe")
        return {
            'ber': 0.5,
            'total_packets': 0,
            'error_stats': {}
        }
    
    total_packets = len(df)
    total_bits = 0
    total_bit_errors = 0
    single_bit_error_count = 0
    parity_bit_error_count = 0
    other_error_count = 0
    burst_error_count = 0
    
    for _, row in df.iterrows():
        # Parse and decode the payload
        encoded_payload = parse_payload(row.payload)
        decoded_payload = hamming74_decode_bytes(encoded_payload)
        
        # Get expected data (you'll need to implement this based on your data format)
        expected_data = get_expected_data(row)  # This function needs to be implemented
        
        # Calculate errors
        errors = count_bit_errors(decoded_payload, expected_data)
        bursts = find_burst_errors(decoded_payload, expected_data)
        
        # Update statistics
        total_bit_errors += errors
        total_bits += len(decoded_payload) * 8
        
        # Classify error type
        if errors == 1:
            single_bit_error_count += 1
        elif errors > 1 and errors % 2 == 0:
            parity_bit_error_count += 1
        else:
            other_error_count += 1
            
        if bursts:
            burst_error_count += 1
    
    # Calculate final statistics
    ber = total_bit_errors / total_bits if total_bits > 0 else 0.0
    
    error_stats = {
        'single_bit_errors': single_bit_error_count,
        'parity_bit_errors': parity_bit_error_count,
        'other_errors': other_error_count,
        'burst_errors': burst_error_count,
        'single_bit_error_percent': (single_bit_error_count / total_packets * 100) if total_packets > 0 else 0,
        'parity_bit_error_percent': (parity_bit_error_count / total_packets * 100) if total_packets > 0 else 0,
        'other_error_percent': (other_error_count / total_packets * 100) if total_packets > 0 else 0,
        'burst_error_percent': (burst_error_count / total_packets * 100) if total_packets > 0 else 0
    }
    
    return {
        'ber': ber,
        'total_packets': total_packets,
        'error_stats': error_stats
    }

def plot_error_analysis(error_stats: Dict):
    """Plot error analysis results"""
    categories = ['Single-bit', 'Parity', 'Other', 'Burst']
    percentages = [
        error_stats['single_bit_error_percent'],
        error_stats['parity_bit_error_percent'],
        error_stats['other_error_percent'],
        error_stats['burst_error_percent']
    ]
    
    plt.figure(figsize=(10, 6))
    plt.bar(categories, percentages)
    plt.title('Error Type Distribution')
    plt.ylabel('Percentage of Packets')
    plt.ylim(0, 100)
    
    # Add percentage labels on top of bars
    for i, v in enumerate(percentages):
        plt.text(i, v + 1, f'{v:.1f}%', ha='center')
    
    plt.show()

def main():
    # Example usage
    # df = pd.read_csv('your_data.csv')
    # results = compute_ber_with_hamming(df)
    # print(f"Bit Error Rate: {results['ber']*100:.2f}%")
    # plot_error_analysis(results['error_stats'])
    pass

if __name__ == "__main__":
    main() 