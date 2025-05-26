# NMv3 Library for Succorfish Delphis Acoustic Modems

This library provides a C++ packet parser and communication interface for Succorfish Delphis acoustic modems. It handles the complete communication cycle: formatting and sending packets that the modem understands, then receiving and parsing response packets from the modem according to the NMv3 protocol for underwater acoustic communication.

**Note:** The header file (`nmv3_api.hpp`) contains complete packet structure definitions for the full Delphis modem protocol specification, including all command types, response formats, and data fields. However, only the essential commands needed for our specific application have been implemented in the C++ code.

## Features

- **Packet Formatting & Transmission**: Creates properly formatted command packets for the modem
- **Response Parsing**: Receives and parses response packets from the modem
- Modem status querying (address and voltage only)
- Basic address configuration
- Simple message broadcasting
- Ping functionality for range measurement

## Supported Commands and Data Fields

The library implements these NMv3 protocol commands with selective data extraction:

### Implemented Commands
- **Query Status (`$?`)**: Extracts modem address and supply voltage *(ignores release info and build time)*
- **Set Address (`$AXXX`)**: Sets and confirms new modem address
- **Broadcast Message**: Sends/receives basic payload *(ignores extended voltage and noise data)*
- **Ping**: Measures range using 1500 m/s sound speed *(ignores link quality indicators)*

### Ignored Protocol Features
Data fields defined in the protocol spec but discarded during parsing:
- Link Quality indicators (quality and Doppler)
- System time data
- Release version and build time information
- Noise measurements and extended voltage data

### Unimplemented Packet Types
The following packet types are fully defined in the header file but not implemented in the C++ code:
- Channel Impulse Response packets
- Echo Message packets
- Unicast with Acknowledgment packets
- Test Message packets
- Spectrum Measurement packets
- Voltage and Noise Measurement packets
- Extension Command packets (System Time, Link Quality control)
- Corrected Errors packets

## Usage

### Initialization

The library requires a `HardwareSerial` connection to the modem. Make sure to set up the serial connection before using the library functions.

```cpp
extern HardwareSerial& modem_connection;
```

### Basic Functions

1. **Query Status** - *Send & Parse*
```cpp
void query_status(void);
```
Sends a status query packet (`$?`) to the modem and parses the response to extract modem address and supply voltage.

2. **Set Address** - *Send & Parse*
```cpp
void set_address(uint8_t addr);
```
Sends an address configuration packet (`$AXXX`) to the modem and parses the confirmation response with the new address.

3. **Broadcast Message** - *Send Only*
```cpp
void broadcast(char *data, uint8_t bytes);
```
Formats and sends a broadcast message packet with the specified payload to all modems. Does not wait for or parse responses.

4. **Ping** - *Send & Parse*
```cpp
void ping(uint8_t addr);
```
Sends a ping packet to the specified modem address and parses the response to extract range measurement data.

5. **Packet Parser** - *Parse Only*
```cpp
ParseResult packet_received_modem(uint8_t* packetBuffer, uint8_t size);
```
Pure parsing function that takes raw packet data received from the modem and returns a structured `ParseResult` containing the parsed packet type and data. Handles all supported packet types including status responses, broadcast messages, ping responses, and error packets.

## Protocol Details

### Packet Parser Functionality

This library acts as a complete packet parser for the NMv3 protocol:

1. **Outgoing Packet Formatting**: Constructs properly formatted command packets with correct prefixes, command types, and payloads that the modem can understand
2. **Incoming Packet Parsing**: Receives response packets from the modem and parses them according to the protocol specification to extract meaningful data
3. **Bidirectional Communication**: Handles the full request-response cycle for each supported command

### Packet Structure

All packets follow a standard format:
- Prefix byte (Command: '$', Response: '#', Error: 'E')
- Command/Response type byte
- Payload (variable length, all defined in `nmv3_api.hpp`)
- *Note: Optional extra data fields (Time, Link Quality) are ignored*

### Maximum Sizes
- Maximum payload size: 64 bytes

## Notes

- Sound speed is fixed at 1500 m/s for range calculations
- All communication is handled through the specified serial connection
- The library handles both local echoes and remote responses
- Only basic packet parsing is implemented, ignoring extended data fields
- Error handling focuses on basic packet validation

