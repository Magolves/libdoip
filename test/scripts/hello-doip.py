import udsoncan
from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.exceptions import *
from udsoncan.services import *

ecu_ip = '127.0.0.1'
ecu_logical_address = 0x00E0
doip_client = DoIPClient(ecu_ip, ecu_logical_address)
conn = DoIPClientUDSConnector(doip_client)

with Client(conn, request_timeout=2) as client:
    try:
        # Option 1: Wait for announcement
        #address, announcement = doip_client.await_vehicle_announcement()
        # Option 2: Send request
        address, announcement = doip_client.get_entity()

        logical_address = announcement.logical_address
        ip, port = address
        print(ip, port, logical_address)
    except TimeoutError as to:
        print("No response from ECU")
    except Exception as e:
        print(f"An error occurred: {e}")
