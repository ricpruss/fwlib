FROM --platform=linux/amd64 python:3.9-slim as builder

WORKDIR /usr/src/fwlib-python-c-extension

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    python3-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the FANUC library
COPY libfwlib32-linux-x64.so.1.0.5 /usr/local/lib/
RUN ln -s /usr/local/lib/libfwlib32-linux-x64.so.1.0.5 /usr/local/lib/libfwlib32.so && \
    ln -s /usr/local/lib/libfwlib32-linux-x64.so.1.0.5 /usr/local/lib/libfwlib32.so.1 && \
    ldconfig

# Install Python build tools
RUN pip install --no-cache-dir setuptools wheel

# Copy the C extension source files
COPY ./examples/python-c-extension/fwlib.c ./examples/python-c-extension/setup.py ./fwlib32.h ./

# Build the C extension
RUN python3 setup.py bdist_wheel

FROM --platform=linux/amd64 python:3.9-slim

WORKDIR /app

# Copy the FANUC library
COPY libfwlib32-linux-x64.so.1.0.5 /usr/local/lib/
RUN ln -s /usr/local/lib/libfwlib32-linux-x64.so.1.0.5 /usr/local/lib/libfwlib32.so && \
    ln -s /usr/local/lib/libfwlib32-linux-x64.so.1.0.5 /usr/local/lib/libfwlib32.so.1 && \
    ldconfig

# Install the C extension
COPY --from=builder /usr/src/fwlib-python-c-extension/dist/*.whl .
RUN pip install --no-cache-dir /app/*.whl && rm /app/*.whl

# Create a directory for Python scripts
RUN mkdir -p /app/python

# Set the working directory to the Python scripts directory
WORKDIR /app/python

# The Python directory will be mounted at runtime
# Use the following command to run the container:
# docker run --rm -it --platform=linux/amd64 -v $(pwd)/examples/python:/app/python fwlib-status python3 scripts/send_mcode.py

CMD ["python3", "scripts/send_mcode.py"]
