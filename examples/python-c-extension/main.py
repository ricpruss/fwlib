from fwlib import Context


# Use the context manager to connect and retrieve the CNC ID
with Context(host="172.18.0.4", port=8193) as cnc:
    cnc_id = cnc.read_id()
    print(f"CNC ID: {cnc_id}")
