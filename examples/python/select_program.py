#!/usr/bin/env python3

import fwlib
import argparse
import re # Import regex module for error parsing
from datetime import datetime

def main():
    parser = argparse.ArgumentParser(description='FANUC CNC Select Main Program')
    parser.add_argument('--host', default='172.18.0.4', help='CNC IP address')
    parser.add_argument('--port', type=int, default=8193, help='CNC port')
    parser.add_argument('--timeout', type=int, default=10, help='Connection timeout in seconds')
    parser.add_argument('program_path', help='Full path of the program to select (e.g., //CNC_MEM/USER/PATH/O1234.NC)')
    args = parser.parse_args()

    print(f"Connecting to CNC at {args.host}:{args.port}...")
    print(f"Attempting to select main program: {args.program_path}")
    print("NOTE: Ensure the CNC is in EDIT mode before running this script.")

    try:
        # Connect using a context manager
        with fwlib.Context(host=args.host, port=args.port, timeout=args.timeout) as cnc:
            cnc_id = cnc.read_id()
            print(f"\nConnected to CNC ID: {cnc_id}")

            # --- Read current main program before selection ---
            try:
                current_path = cnc.read_main_program_path()
                print(f"Current main program before selection: {current_path}")
            except Exception as e:
                print(f"Warning: Could not read current main program path: {e}")

            # --- Attempt to select the new main program ---
            print(f"\nSending command to select '{args.program_path}'...")
            cnc.select_main_program(args.program_path)
            print("Selection command sent successfully.")

            # --- Verify the selection ---
            print("\nVerifying selection...")
            try:
                selected_path = cnc.read_main_program_path()
                print(f"Current main program after selection: {selected_path}")
                if selected_path == args.program_path:
                    print("Verification successful: Path matches the requested path.")
                else:
                    print(f"Verification FAILED: Path '{selected_path}' does not match requested path '{args.program_path}'.")
            except Exception as e:
                print(f"Error reading main program path after selection: {e}")
                print("Could not verify selection.")

    except ConnectionError as e:
        print(f"\nConnection Error: {e}")
    except RuntimeError as e:
        # Specific error from the fwlib C extension (e.g., selection failed)
        print(f"\nRuntime Error during selection: {e}")
        print("Selection likely failed. Check CNC state (EDIT mode?) and path.")

        # Attempt to get detailed error information from the CNC
        try:
            # Check if the Context object 'cnc' exists 
            # (it should exist if we are in this block after the 'with' statement succeeded)
            if 'cnc' in locals(): 
                 # Parse the original FOCAS error code from the exception message
                match = re.search(r': (\d+)$', str(e))
                if match:
                    original_error_code = int(match.group(1))
                    print(f"Original FOCAS error code: {original_error_code}")

                    # For cnc_pdf_slctmain, error code 5 (EW_PATH/EW_DATA) has details
                    if original_error_code == 5:
                        print("Attempting to get detailed error info for error code 5...")
                        detailed_error = cnc.get_detailed_error()
                        print(f"Detailed Error Info: {detailed_error}")
                        detail_code = detailed_error.get('detail_error_code', 'N/A')
                        if detail_code == 1:
                            print("  -> Detail Code 1: Format error in path/filename.")
                        elif detail_code == 2:
                            print("  -> Detail Code 2: Specified file cannot be found at that path.")
                        else:
                            print(f"  -> Unknown detail code: {detail_code}")
                    else:
                        print(f"Detailed error info not typically available or needed for error code {original_error_code}.")
                else:
                    print("Could not parse original FOCAS error code from the message.")
            else:
                 print("CNC context not available (unexpected error). Cannot fetch detailed error.")

        except Exception as detail_e:
            # Handle errors during the detailed error fetching process itself
            print(f"Could not get or process detailed error info: {detail_e}")

    except Exception as e:
        print(f"\nAn unexpected error occurred: {e}")

if __name__ == "__main__":
    main() 