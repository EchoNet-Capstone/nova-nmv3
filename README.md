# NMv3 Library for Succorfish Delphis Acoustic Modems

This library provides a C++ interface for communicating with Succorfish Delphis acoustic modems. It implements a subset of the NMv3 protocol for underwater acoustic communication.

## Features

- Modem status querying (address and voltage only)
- Basic address configuration
- Simple message broadcasting
- Ping functionality for range measurement

## Supported Commands and Data Fields

The library implements a subset of the protocol's capabilities:

### Query Status (`$?`)
- Returns: 
  - Modem address
  - Supply voltage
  - *Note: Release info and build time fields are ignored*

### Set Address (`$AXXX`)
- Sets modem address
- Returns confirmation with new address

### Broadcast Message
- Sends basic payload to all modems
- Returns source address and payload
- *Note: Extended voltage and noise data variants are ignored*

### Ping
- Measures range to another modem
- Returns source address and range data
- Uses 1500 m/s sound speed for calculations

### Ignored Protocol Features
The following data fields from the protocol spec are not processed, but easily can be if needed:
- Link Quality indicators (quality and Doppler)
- System time data
- Release version information
- Build time information
- Noise measurements
- Extended voltage data

## Usage

### Initialization

The library requires a `HardwareSerial` connection to the modem. Make sure to set up the serial connection before using the library functions.

```cpp
extern HardwareSerial& modem_connection;
```

### Basic Functions

1. **Query Status**
```cpp
void query_status(void);
```

2. **Set Address**
```cpp
void set_address(uint8_t addr);
```

3. **Broadcast Message**
```cpp
void broadcast(char *data, uint8_t bytes);
```

4. **Ping**
```cpp
void ping(uint8_t addr);
```

## Protocol Details

### Packet Structure

All packets follow a standard format:
- Prefix byte (Command: '$', Response: '#', Error: 'E')
- Command/Response type byte
- Payload (variable length)
- *Note: Optional extra data fields (Time, Link Quality) are ignored*

### Maximum Sizes
- Maximum payload size: 64 bytes
- Maximum address size: 3 bytes
- Maximum data size: 2 bytes

## Notes

- Sound speed is fixed at 1500 m/s for range calculations
- All communication is handled through the specified serial connection
- The library handles both local echoes and remote responses
- Only basic packet parsing is implemented, ignoring extended data fields
- Error handling focuses on basic packet validation

For complete protocol specifications including unimplemented features, refer to the header files.
