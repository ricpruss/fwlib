#!/usr/bin/env python3

import argparse
import traceback
from fwlib import Context

# PMC Address Type Constants (Copied from read_pmc.py for standalone use)
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

def write_and_verify_pmc(host, port, value_to_write):
    """
    Connects to the CNC, writes a PMC value, and verifies the write.

    Args:
        host (str): The CNC host IP address
        port (int): The CNC port number
        value_to_write (int): The byte value to write to PMC Y10.
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

            # --- Write PMC Y10 (byte) ---
            pmc_addr_type = PMC_ADDR_Y
            pmc_data_type = PMC_TYPE_BYTE
            pmc_start_num = 10
            pmc_end_num = 10
            # value_to_write = 1 # Set Y10.0 high, rest low <- Removed hardcoding

            # Validate the value for byte type (0-255)
            if not 0 <= value_to_write <= 255:
                print(f"Error: Value {value_to_write} is out of range for a byte (0-255).")
                return # Exit the function

            print(f"\nAttempting to write PMC Y{pmc_start_num} (byte) with value {value_to_write}...")
            
            try:
                # Data must be provided as a list or tuple matching the range size
                data_list = [value_to_write] 
                print(f"Data list: {data_list}")
                print(f"PMC address type: {pmc_addr_type}")
                print(f"PMC data type: {pmc_data_type}")
                print(f"PMC start number: {pmc_start_num}")
                print(f"PMC end number: {pmc_end_num}")
                cnc.write_pmc(pmc_addr_type, pmc_data_type, pmc_start_num, pmc_end_num, data_list)
                print("Write command sent successfully.")

                # --- Verify the write ---
                print("Verifying write by reading back...")
                read_back_value = cnc.read_pmc(pmc_addr_type, pmc_data_type, pmc_start_num, pmc_end_num)[0]
                print(f"Read back Y{pmc_start_num}: {read_back_value}")

                if read_back_value == value_to_write:
                    print("Verification successful: Read-back value matches written value.")
                else:
                    print(f"Verification FAILED: Read-back value ({read_back_value}) does not match written value ({value_to_write}).")
            
            except RuntimeError as e:
                 print(f"Runtime Error during PMC write/read: {e}")
                 # You might want to check cnc_getdtailerr here if applicable for pmc_wrpmcrng errors
                 traceback.print_exc()
            except Exception as e:
                print(f"An unexpected error occurred during write/verification: {e}")
                traceback.print_exc()

    except ConnectionError as e:
        print(f"Connection error: {e}")
    except Exception as e:
        print(f"Unexpected error during connection: {type(e).__name__}: {e}")
        traceback.print_exc()

if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Write PMC values to FANUC CNC')
    parser.add_argument('--host', default='172.18.0.4', help='CNC IP address')
    parser.add_argument('--port', type=int, default=8193, help='CNC port')
    parser.add_argument('--timeout', type=int, default=10, help='Connection timeout in seconds') # Added timeout
    parser.add_argument('--value', type=int, default=0, help='Byte value to write to Y10 (0-255)') # New argument
    args = parser.parse_args()
    
    # Write and verify PMC values
    write_and_verify_pmc(args.host, args.port, args.value) # Pass the value argument 