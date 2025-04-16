# FANUC Fwlib Python Scripts

This directory contains Python scripts that use the FANUC Fwlib C extension to communicate with CNC machines.

## Directory Structure

- `scripts/`: Contains Python scripts for various CNC operations
  - `send_mcode.py`: Script to send M-codes to the CNC machine

## Running Scripts with Docker

To run a script using Docker, use the following command:

```bash
docker run --rm -it --platform=linux/amd64 \
  -v $(pwd)/examples/python:/app/python \
  fwlib-status \
  python3 scripts/send_mcode.py
```

Replace `send_mcode.py` with the script you want to run.

## Adding New Scripts

To add a new script:

1. Create a new Python file in the `scripts/` directory
2. Import the `Context` class from the `fwlib` module
3. Use the `Context` class to communicate with the CNC machine

Example:

```python
from fwlib import Context

def my_function():
    with Context(host="172.18.0.4", port=8193) as cnc:
        # Your code here
        pass

if __name__ == "__main__":
    my_function() 