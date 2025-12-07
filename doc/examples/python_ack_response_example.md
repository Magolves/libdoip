# Python example: handling DoIP ack vs downstream response

This short example shows how a DoIP/UDS client should treat the transport-level
Diagnostic Ack (payload type 0x8002) and the functional UDS response which
arrives later as a Diagnostic Message (payload type 0x8001).

The `change-vin-and-reset.py` script in `test/scripts/` already demonstrates
an end-to-end flow: it sends UDS requests and waits for functional responses.

Key points for client implementers:

- The DoIP server sends an immediate Diagnostic Ack (0x8002) after receiving
  a Diagnostic Message (0x8001). This confirms transport reception only.
- The functional UDS response may arrive later as another Diagnostic Message
  (0x8001). The client must wait for that message to obtain the service
  result.
- When using `udsoncan`, the library abstracts away DoIP transport details.
  If you implement a custom connector, ensure you forward both the ack and
  subsequent response frames appropriately.

Minimal pseudo-code (conceptual):

```python
# Using doipclient + udsoncan
from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client

client = DoIPClient('127.0.0.1', 0x00E0)
conn = DoIPClientUDSConnector(client)

with Client(conn, request_timeout=2) as u:
    # change session -> server will ack on transport, then send response
    resp = u.change_session(0x01)
    # udsoncan waits for the real UDS response; transport ACK is handled internally
    print('Session change response:', resp)
```

If you handle sockets yourself, inspect packet payload types: treat 0x8002
as transport-level ack and do not consider it the functional response.

```python
# Pseudocode for low-level handling
sock = setup_doip_socket()
while True:
    pkt = sock.recv()
    payload_type = parse_payload_type(pkt)
    if payload_type == 0x8002:
        # transport ack - ignore for UDS result
        continue
    if payload_type == 0x8001:
        # functional UDS response - process
        handle_uds_response(pkt)
```

For completeness, see `test/scripts/change-vin-and-reset.py` which performs a
full read/write VIN and ECU reset sequence using `udsoncan` with the DoIP
connector.
