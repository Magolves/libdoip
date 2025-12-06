import udsoncan
from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.exceptions import *
from udsoncan.services import *
from udsoncan import DataIdentifier, AsciiCodec

udsoncan.setup_logging()

# Add this config
config = {
    'data_identifiers': {
        DataIdentifier.VIN: AsciiCodec(17)
    }
}

ecu_ip = '127.0.0.1'
ecu_logical_address = 0x00E0
doip_client = DoIPClient(ecu_ip, ecu_logical_address)
conn = DoIPClientUDSConnector(doip_client)
with Client(conn, request_timeout=2, config=config) as client:
   try:
      client.change_session(1)  # integer with value of 3
      #client.unlock_security_access(MyCar.debug_level)                                   # Fictive security level. Integer coming from fictive lib, let's say its value is 5
      # Read VIN once and handle response type (udsoncan may return bytes or str)
      vin_response = client.read_data_by_identifier(udsoncan.DataIdentifier.VIN)
      vin_value = vin_response.service_data.values[udsoncan.DataIdentifier.VIN]
      if isinstance(vin_value, bytes):
         vin_str = vin_value.decode('ascii', errors='ignore')
      else:
         # already a str
         vin_str = str(vin_value)
      print('Current Vehicle Identification Number is: %s' % vin_str)

      client.write_data_by_identifier(udsoncan.DataIdentifier.VIN, 'ABC123456789GHIJK')       # Standard ID for VIN is 0xF190. Codec is set in the client configuration
      print('Vehicle Identification Number successfully changed.')
      client.ecu_reset(ECUReset.ResetType.hardReset)                                     # HardReset = 0x01
   except NegativeResponseException as e:
      print('Server refused our request for service %s with code "%s" (0x%02x)' % (e.response.service.get_name(), e.response.code_name, e.response.code))
   except (InvalidResponseException, UnexpectedResponseException) as e:
      print('Server sent an invalid payload : %s' % e.response.original_payload)

   # Because we reset our UDS server, we must also reconnect/reactivate the DoIP socket
   # Alternatively, we could have used the auto_reconnect_tcp flag on the DoIPClient
   # Note: ECU's do not restart instantly, so you may need a sleep() before moving on
   doip_client.reconnect()
   client.tester_present()

# Cleanup the DoIP Socket when we're done. Alternatively, we could have used the
# close_connection flag on conn so that the udsoncan client would clean it up
doip_client.close()