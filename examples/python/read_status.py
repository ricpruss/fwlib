#!/usr/bin/env python3

import fwlib
import time
import argparse
from datetime import datetime

def print_dict(title, data):
    print(f"\n{title}:")
    print("-" * (len(title) + 1))
    for key, value in data.items():
        if isinstance(value, list):
            # Handle position data which is now a list per axis
            print(f"{key}:")
            for i, val in enumerate(value):
                print(f"  Axis {i}: {val}")
        else:
            print(f"{key}: {value}")

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='FANUC CNC Status Monitor')
    parser.add_argument('--host', default='172.18.0.4', help='CNC IP address')
    parser.add_argument('--port', type=int, default=8193, help='CNC port')
    parser.add_argument('--timeout', type=int, default=10, help='Connection timeout in seconds')
    parser.add_argument('--interval', type=float, default=1.0, help='Update interval in seconds')
    args = parser.parse_args()
    
    print(f"Connecting to CNC at {args.host}:{args.port}...")
    
    # Connect to the CNC
    with fwlib.Context(host=args.host, port=args.port, timeout=args.timeout) as cnc:
        try:
            # Read CNC ID
            cnc_id = cnc.read_id()
            print(f"\nCNC ID: {cnc_id}")
            
            while True:
                # Clear screen (works on both Windows and Unix-like systems)
                print("\033[H\033[J", end="")
                
                # Print timestamp
                print(f"Status Update: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
                print(f"Connected to: {args.host}:{args.port}")
                
                # Read and display status information
                status = cnc.read_status()
                print_dict("Machine Status", status)
                
                # Read and display position information
                position = cnc.read_position()
                print_dict("Position Information", position)
                
                # Read and display spindle information
                spindle = cnc.read_spindle()
                print_dict("Spindle Information", spindle)
                
                # Wait for the specified interval before next update
                time.sleep(args.interval)
                
        except KeyboardInterrupt:
            print("\nStopping status monitor...")
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    main() 