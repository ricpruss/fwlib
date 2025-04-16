#!/usr/bin/env python3

import fwlib
import argparse
from datetime import datetime
import time

def print_dict(title, data):
    print(f"\n{title}:")
    print("-" * (len(title) + 1))
    for key, value in data.items():
        print(f"{key}: {value}")

def main():
    parser = argparse.ArgumentParser(description='FANUC CNC Program Number Monitor')
    parser.add_argument('--host', default='172.18.0.4', help='CNC IP address')
    parser.add_argument('--port', type=int, default=8193, help='CNC port')
    parser.add_argument('--timeout', type=int, default=10, help='Connection timeout in seconds')
    args = parser.parse_args()

    print(f"Connecting to CNC at {args.host}:{args.port}...")

    try:
        # Connect using a context manager
        with fwlib.Context(host=args.host, port=args.port, timeout=args.timeout) as cnc:
            cnc_id = cnc.read_id()
            print(f"\nCNC ID: {cnc_id}")

            # Clear screen (works on Unix-like systems and modern Windows terminals)
            print("\033[H\033[J", end="") 

            print(f"Program Number Update: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"Connected to: {args.host}:{args.port} (ID: {cnc_id})")

            # Read program numbers
            try:
                program_info = cnc.read_program_number()
                print_dict("Program Information", program_info)
            except Exception as e:
                print(f"\nError reading program number: {e}")
                

    except KeyboardInterrupt:
        print("\nStopping program number monitor...")
    except ConnectionError as e:
        print(f"Connection Error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    main() 