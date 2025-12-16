Based on the ISO 14229 standard, here are the **standardized Data Identifiers (DIDs)** defined for the ReadDataByIdentifier (RDBI - 0x22) service:

## **DID Range Allocation in ISO 14229**

The standard defines specific ranges for different purposes:

### **ISO 14229-1:2020 / 2013 DID Ranges:**

**Reserved & Standardized Ranges:**
- **0x0000-0x00FF**: Reserved for future ISO/SAE definition
- **0xA600-0xA7FF**: Reserved for legislative use
- **0xAD00-0xAFFF**: Reserved for legislative use
- **0xB200-0xBFFF**: Reserved for legislative use
- **0xC300-0xCEFF**: Reserved for legislative use

**Vehicle Manufacturer Specific Ranges:**
- 0x0100-0xA5FF: Vehicle manufacturer specific
- 0xA800-0xACFF: Vehicle manufacturer specific
- 0xB000-0xB1FF: Vehicle manufacturer specific
- 0xC000-0xC2FF: Vehicle manufacturer specific
- 0xCF00-0xEFFF: Vehicle manufacturer specific
- 0xF010-0xF0FF: Vehicle manufacturer specific

### **Standardized DIDs (0xF000-0xFFFF range):**

These are the **actually standardized DIDs** that ISO 14229 defines:

**Network Configuration (Tractor-Trailer):**
- 0xF000-0xF00F: Network configuration data for tractor-trailer application

**Vehicle/ECU Identification:**
- 0xF100-0xF17F: Identification option vehicle manufacturer specific data
- 0xF180: Boot software identification
- 0xF181: Application software identification
- 0xF182: Application data identification
- **0xF183**: Boot software fingerprint (last bootloader update info)
- 0xF184: Application software fingerprint
- 0xF185: Application data fingerprint
- **0xF186**: Active diagnostic session
- **0xF187**: Vehicle manufacturer spare part number
- **0xF188**: Vehicle manufacturer ECU software number
- **0xF189**: Vehicle manufacturer ECU software version number
- **0xF18A**: System supplier identifier
- **0xF18B**: ECU manufacturing date and time
- **0xF18C**: System supplier ECU hardware number
- **0xF18D**: System supplier ECU hardware version number
- **0xF18E**: System supplier ECU software number
- **0xF18F**: System supplier ECU software version number
- **0xF190**: Exhaust regulation or type approval number
- **0xF191**: System name or engine type
- **0xF192**: Repair shop code or tester serial number
- **0xF193**: Programming date
- **0xF194**: Calibration repair shop code or calibration equipment serial number
- **0xF195**: Calibration date
- **0xF196**: Calibration equipment software number
- **0xF197**: ECU installation date
- **0xF198**: ODX file data identifier
- **0xF199**: Entity data identifier

**VIN and Additional Info:**
- **0xF19A**: Network configuration data identifier
- **0xF19B-0xF19D**: Reserved
- **0xF19E**: VIN (Vehicle Identification Number)
- **0xF19F-0xF1EF**: Various manufacturer-specific identifiers

**Diagnostic and Communication:**
- **0xF1F0-0xF1FF**: Diagnostic and communication management
- **0xF200-0xFFFF**: Additional ranges for various purposes

## **Key Points:**

1. **Most DIDs are manufacturer-specific** - The majority of the DID range (0x0100-0xEFFF) is reserved for vehicle manufacturers to define according to their needs.

2. **Standardized DIDs start at 0xF000** - The truly standardized DIDs that are defined across all implementations are primarily in the 0xF100-0xF1FF range.

3. **Common standardized DIDs** include:
   - VIN (0xF19E)
   - Software identification (0xF180, 0xF181, 0xF182)
   - ECU manufacturing info (0xF18B)
   - Hardware/software version numbers (0xF188-0xF18F)

4. **DIDs are 2 bytes** (16-bit values from 0x0000 to 0xFFFF), allowing for 65,536 possible identifiers.

The standard intentionally leaves most of the DID space for manufacturers to customize according to their specific vehicle architectures and diagnostic requirements, while providing a common set of essential identifiers for basic ECU information and identification.