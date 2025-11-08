#include <iostream>
#include "ByteArray.h"

using namespace doip;

int main() {
    // Example 1: Simple ByteArray
    ByteArray data1 = {0x01, 0x02, 0x03};
    std::cout << "data1: " << data1 << std::endl;
    // Output: data1: 01.02.03

    // Example 2: DoIP header
    ByteArray doip_header = {0x02, 0xFD, 0x80, 0x01, 0x00, 0x00, 0x00, 0x04};
    std::cout << "DoIP header: " << doip_header << std::endl;
    // Output: DoIP header: 02.FD.80.01.00.00.00.04

    // Example 3: ASCII characters
    ByteArray ascii = {'H', 'e', 'l', 'l', 'o'};
    std::cout << "ASCII as hex: " << ascii << std::endl;
    // Output: ASCII as hex: 48.65.6C.6C.6F

    // Example 4: Mixed with other output
    ByteArray vin = {0x57, 0x56, 0x57, 0x5A, 0x5A, 0x5A, 0x31, 0x4A};
    std::cout << "VIN bytes: " << vin << " (" << vin.size() << " bytes)" << std::endl;
    // Output: VIN bytes: 57.56.57.5A.5A.5A.31.4A (8 bytes)

    // Example 5: Empty array
    ByteArray empty;
    std::cout << "Empty: [" << empty << "]" << std::endl;
    // Output: Empty: []

    return 0;
}
