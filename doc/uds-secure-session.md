The ISO 14229 standard, also known as the Unified Diagnostic Services (UDS) protocol, defines a communication protocol for diagnostic sessions in automotive systems. The flow of a security diagnostic session typically follows these steps:
 1.
Session Establishment:
•
The diagnostic tester (client) sends a request to establish a diagnostic session with the vehicle's Electronic Control Unit (ECU).
•
The ECU responds with an acknowledgment or rejection.
 2.
Authentication:
•
The tester and ECU perform an authentication process to ensure that only authorized entities can access diagnostic functions.
•
This may involve exchanging security keys or certificates.
 3.
Security Access:
•
The tester requests security access to perform specific diagnostic functions.
•
The ECU grants or denies access based on the authentication results.
 4.
Diagnostic Functions:
•
Once security access is granted, the tester can perform diagnostic functions such as reading fault codes, clearing fault codes, or reprogramming the ECU.
 5.
Session Termination:
•
The diagnostic session is terminated, either by the tester or the ECU, to ensure that no unauthorized access remains.

Detailed Flow:
 1.
Diagnostic Session Control (Service 0x10):
•
The tester sends a request to start a diagnostic session (e.g., 0x10 0x01 for a default session or 0x10 0x03 for an extended session).
•
The ECU responds with a positive or negative acknowledgment.
 2.
Security Access (Service 0x27):
•
The tester sends a request for security access (e.g., 0x27 0x01 to request a seed).
•
The ECU responds with a seed value.
•
The tester calculates a key based on the seed and sends it to the ECU (e.g., 0x27 0x02 <key>).
•
The ECU verifies the key and grants or denies access.
 3.
Diagnostic Functions:
•
The tester can now perform diagnostic functions such as:
▪
Reading Data by Identifier (Service 0x22).
▪
Writing Data by Identifier (Service 0x2E).
▪
Clearing Diagnostic Information (Service 0x14).
▪
Reprogramming the ECU (Service 0x34, 0x36, etc.).
 4.
Session Termination:
•
The tester sends a request to end the diagnostic session (e.g., 0x10 0x01 to return to the default session).
•
The ECU acknowledges the session termination.
Example:
Here is an example of a security diagnostic session flow:
 1.
Start Diagnostic Session:
•
Tester: 0x10 0x03 (Request extended diagnostic session)
•
ECU: 0x50 0x03 (Positive response)
 2.
Request Security Access:
•
Tester: 0x27 0x01 (Request seed)
•
ECU: 0x67 0x01 <seed> (Response with seed)
•
Tester: 0x27 0x02 <key> (Send calculated key)
•
ECU: 0x67 0x02 (Positive response, security access granted)
 3.
Perform Diagnostic Function:
•
Tester: 0x22 0xF190 (Read data by identifier)
•
ECU: 0x62 0xF190 <data> (Response with data)
 4.
End Diagnostic Session:

 1.
XOR Operation:
•
A simple XOR operation between the seed and a fixed secret value.
•
Example: If the seed is 0x12345678 and the secret is 0xA5A5A5A5, the key would be 0x12345678 ^ 0xA5A5A5A5 = 0xB791F32D.
 2.
Hash Functions:
•
More secure algorithms like SHA-256 or SHA-1 may be used to generate the key from the seed.
•
Example: The key is the SHA-256 hash of the seed concatenated with a secret.
 3.
Symmetric Encryption:
•
Algorithms like AES (Advanced Encryption Standard) may be used to encrypt the seed with a secret key to generate the response.
•
Example: The key is the AES-encrypted value of the seed using a predefined key.
 4.
Asymmetric Encryption:
•
Public-key cryptography (e.g., RSA) may be used, where the ECU holds a private key and the tester uses a public key to generate the response.
•
Example: The key is the RSA-encrypted value of the seed using the ECU's public key.
Example Flow:
Here is an example of a UDS security access flow using a simple XOR operation:
 1.
Request Seed:
•
Tester: 0x27 0x01 (Request seed for security level 1)
•
ECU: 0x67 0x01 0x12 0x34 0x56 0x78 (Response with seed 0x12345678)
 2.
Calculate Key:
•
The tester calculates the key using a predefined secret (e.g., 0xA5A5A5A5):
Key = Seed ^ Secret
Key = 0x12345678 ^ 0xA5A5A5A5 = 0xB791F32D
 3.
Send Key:
•
Tester: 0x27 0x02 0xB7 0x91 0xF3 0x2D (Send calculated key)
•
ECU: 0x67 0x02 (Positive response, security access granted)
Notes:
•
The actual seeds, keys, and algorithms are proprietary and vary by manufacturer.
•
Modern vehicles often use more complex and secure algorithms to prevent unauthorized access.
•
The examples provided are simplified for illustrative purposes. Real-world implementations may involve more complex cryptographic operations.