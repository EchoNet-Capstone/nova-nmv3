# NMv3 Library for Succorfish Delphis Acoustic Modems

This library provides a C++ interface for communicating with Succorfish Delphis acoustic modems. It implements the NMv3 protocol for underwater acoustic communication.

## Features

- Modem status querying
- Address configuration
- Message broadcasting
- Unicast messaging with acknowledgment
- Ping functionality for range measurement
- Battery voltage measurement
- Error handling and timeout management

## Supported Commands

The library supports the following commands:

- Query Status (`$?`)
- Set Address (`$AXXX`)
- Broadcast Message
- Unicast Message with Acknowledgment
- Ping
- Unicast Message
- Battery Voltage Measurement (with Query Status)
- Error Handling
- Timeout Management

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

### Response Handling

The library includes a comprehensive packet parser that handles various response types:
- Status query responses
- Address setting confirmations
- Broadcast message responses
- Ping responses with range data
- Unicast message responses
- Error packets

## Protocol Details

### Packet Structure

All packets follow a standard format:
- Prefix byte (Command: '$', Response: '#', Error: 'E')
- Command/Response type byte
- Payload (variable length)
- Optional extra data (Time)

### Maximum Sizes
- Maximum payload size: 64 bytes
- Maximum address size: 3 bytes
- Maximum data size: 2 bytes

## Debug Mode

Debug output can be enabled by defining `DEBUG_ON`. When enabled, the library will output detailed information about packet processing and communication events.

## Dependencies

- Arduino framework (when used with Arduino)
- Standard C++ libraries

## Notes

- Sound speed is configured at 1500 m/s for range calculations
- All communication is handled through the specified serial connection
- The library handles both local echoes and remote responses
- Battery voltage measurement is available through status query
- Packet parsing is done automatically with error checking

## Error Handling

The library includes robust error handling:
- Invalid packet detection
- Size validation
- Type checking
- Response validation

For detailed protocol specifications and implementation details, refer to the header files and source code documentation.
