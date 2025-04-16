#!/usr/bin/env python3

import argparse
import traceback
from fwlib import Context

# PMC Address Type Constants
PMC_ADDR_G = 0  # Output signal from PMC to CNC (G)
PMC_ADDR_F = 1  # Input signal to PMC from CNC (F)
PMC_ADDR_Y = 2  # Output signal from PMC to machine (Y)
PMC_ADDR_X = 3  # Input signal to PMC from machine (X)
PMC_ADDR_A = 4  # Message display (A)
PMC_ADDR_R = 5  # Internal relay (R)
PMC_ADDR_T = 6  # Timer (T)
PMC_ADDR_K = 7  # Keep relay (K)
PMC_ADDR_C = 8  # Counter (C)
PMC_ADDR_D = 9  # Data table (D)
PMC_ADDR_M = 10 # Input signal from other PMC path (M)
PMC_ADDR_N = 11 # Output signal to other PMC path (N)
PMC_ADDR_E = 12 # Extended relay (E)
PMC_ADDR_Z = 13 # System relay (Z)

# PMC Data Type Constants
PMC_TYPE_BYTE = 0   # 8-bit
PMC_TYPE_WORD = 1   # 16-bit
PMC_TYPE_LONG = 2   # 32-bit
PMC_TYPE_FLOAT = 4  # 32-bit floating point
PMC_TYPE_DOUBLE = 5 # 64-bit floating point

def read_pmc_values(host, port):
    """
    Read PMC values from the CNC machine.
    
    Args:
        host (str): The CNC host IP address
        port (int): The CNC port number
    """
    print(f"Connecting to CNC at {host}:{port}")
    try:
        # Create a connection to the CNC
        with Context(host=host, port=port) as cnc:
            print("Successfully connected to CNC")
            
            # Read CNC ID for verification
            try:
                cnc_id = cnc.read_id()
                print(f"Connected to CNC with ID: {cnc_id}")
            except Exception as e:
                print(f"Warning: Could not read CNC ID: {e}")
            
            # Read PMC X7 byte (to get X7.6 and X7.7)
            # For X address, the address type is 3
            print("\nReading PMC X7 (byte with X7.0 - X7.7)")
            try:
                # Read X7 as a byte (all 8 bits)
                x7_value = cnc.read_pmc(PMC_ADDR_X, PMC_TYPE_BYTE, 7, 7)[0]
                print(f"X7 complete byte: {x7_value} (0x{x7_value:02X})")
                
                # Extract individual bits
                for bit in range(8):
                    bit_value = (x7_value >> bit) & 0x01
                    print(f"X7.{bit}: {bit_value}")
                
                # Specifically show X7.6 and X7.7
                print("\nRequested specific bits:")
                print(f"X7.6: {(x7_value >> 6) & 0x01}")
                print(f"X7.7: {(x7_value >> 7) & 0x01}")
            except Exception as e:
                print(f"Error reading PMC X7: {e}")
                traceback.print_exc()
            
            # Alternative method: read individual bits directly
            print("\nReading individual bits using read_pmc_bit:")
            try:
                x7_6_value = cnc.read_pmc_bit(PMC_ADDR_X, 7, 6)
                x7_7_value = cnc.read_pmc_bit(PMC_ADDR_X, 7, 7)
                print(f"X7.6: {x7_6_value}")
                print(f"X7.7: {x7_7_value}")
            except Exception as e:
                print(f"Error reading PMC bits: {e}")
                traceback.print_exc()
                
            # Read multiple addresses for demonstration
            print("\nReading a range of X addresses (X0-X10)")
            try:
                x_values = cnc.read_pmc(PMC_ADDR_X, PMC_TYPE_BYTE, 0, 10)
                for i, value in enumerate(x_values):
                    print(f"X{i}: {value} (0x{value:02X})")
            except Exception as e:
                print(f"Error reading PMC X range: {e}")
                traceback.print_exc()
                
    except ConnectionError as e:
        print(f"Connection error: {e}")
    except Exception as e:
        print(f"Unexpected error: {type(e).__name__}: {e}")
        traceback.print_exc()

if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Read PMC values from FANUC CNC')
    parser.add_argument('--host', default='172.18.0.4', help='CNC IP address')
    parser.add_argument('--port', type=int, default=8193, help='CNC port')
    args = parser.parse_args()
    
    # Read PMC values
    read_pmc_values(args.host, args.port) 