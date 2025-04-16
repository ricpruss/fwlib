#!/bin/bash

# Exit on error
set -e

# Default CNC IP address
CNC_IP=${1:-172.18.0.4}
# Default script name
SCRIPT_NAME=${2:-read_status.py}

# Build the Docker image
echo "Building Docker image..."
docker build --platform=linux/amd64 -f examples/python-c-extension/Dockerfile -t fwlib-status .

# Run the container with network access to the CNC
echo "Running container with CNC IP: $CNC_IP and script: $SCRIPT_NAME..."
docker run --rm -it --platform=linux/amd64 \
  --network host \
  -v $(pwd)/examples/python:/app/scripts \
  fwlib-status python /app/scripts/$SCRIPT_NAME --host $CNC_IP