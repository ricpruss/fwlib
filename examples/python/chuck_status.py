#!/usr/bin/env python3

import argparse
import traceback
from fwlib import Context

# PMC Address Type Constants
PMC_ADDR_X = 3  # Input signal to PMC from machine (X)

def check_chuck_status(host, port):
    """
    Read X7.6 and X7.7 registers to determine chuck status.
    
    Args:
        host (str): The CNC host IP address
        port (int): The CNC port number
    """
    print(f"Connecting to CNC at {host}:{port}")
    try:
        # Create a connection to the CNC
        with Context(host=host, port=port) as cnc:
            print("Successfully connected to CNC")
            
            # Read X7.6 and X7.7 bits directly
            try:
                x7_6_value = cnc.read_pmc_bit(PMC_ADDR_X, 7, 6)
                x7_7_value = cnc.read_pmc_bit(PMC_ADDR_X, 7, 7)
                
                # Determine chuck status based on bit values
                if x7_6_value and not x7_7_value:
                    print("Chuck Open")
                elif not x7_6_value and x7_7_value:
                    print("Chuck Closed")
                else:
                    # Both True or both False
                    print("Chuck Moving or Error")
                    
            except Exception as e:
                print(f"Error reading PMC bits: {e}")
                traceback.print_exc()
                
    except ConnectionError as e:
        print(f"Connection error: {e}")
    except Exception as e:
        print(f"Unexpected error: {type(e).__name__}: {e}")
        traceback.print_exc()

if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Check chuck status from FANUC CNC')
    parser.add_argument('--host', default='172.18.0.4', help='CNC IP address')
    parser.add_argument('--port', type=int, default=8193, help='CNC port')
    args = parser.parse_args()
    
    # Check chuck status
    check_chuck_status(args.host, args.port) 