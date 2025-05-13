#include <string.h>

#include <Arduino.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "nmv3_api.hpp"
#include "neighbor.hpp"
#include "bloomfilter.hpp"

#define SOUND_SPEED 1500

static const ParseResult kNoneResult = { .type = NONE_TYPE };

// Supported Modem Commands (Link Quality Indicator OFF)
//  Query Status                                DONE
//  Set Address                                 DONE
//  Broadcast Message                           DONE
//  Unicast Message with Ack                    DONE
//  Ping                                        DONE
//  Unicast Message                             DONE
//  Battery Voltage and Noise Measurement*      TBD
//  Error
//  Timeout

// ----- Helper functions -----

static
int
fieldToInt(
    char* field,
    uint8_t field_len
){
    char temp[field_len + 1] = {0};

    memcpy(temp, field, field_len);
    temp[field_len] = '\0';

    return atoi(temp);
}

static
void
printBufferContents(
    uint8_t* buf,
    uint8_t size
){
    if (size == 0) Serial.printf("\tOops! This buffer is empty!\r\n"); return;

    Serial.printf("\tBuffer Contents (%03u bytes): \r\n", size);
    Serial.printf("\t    |     0          1          2          3          4          5          6          7\r\n");

    int i = 0;
    for (; i < size - 1; i++) {
        if (i % 8 == 0)
            Serial.printf("\r\n\t%2i |", i / 8);

        Serial.printf("%c (%03u), ", (char)buf[i], buf[i]);
    }

    // Final byte — no trailing comma
    if (i % 8 == 0)
        Serial.printf("\r\n\t%2i |", i / 8);

    Serial.printf("%c (%03u)\r\n", (char)buf[i], buf[i]);
}

// BEGIN MODEM SERIAL CONNECTION FUNCTIONS -----------------
uint8_t modem_id = 0;

uint8_t
get_modem_id(
    void
){
    return modem_id;
}

void
set_modem_id(
    uint8_t new_modem_id
){
    modem_id = new_modem_id;
}

void
query_status(
    HardwareSerial connection
){
    connection.print("$?");
}

void
set_address(
    HardwareSerial connection,
    uint8_t addr
){
    connection.printf("$A%03d", addr);
}

void
broadcast(
    HardwareSerial connection,
    char *data,
    uint8_t bytes
){
    ModemPacket_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.type = MODEM_COMMAND_TYPE;
    pkt.payload.command.type = BROADCAST_CMD_TYPE;

    char temp[BROADCAST_CMD_DATA_SIZE_MAX + 1] = {0};
    snprintf(temp, sizeof(temp), "%02u", bytes);

    memcpy(pkt.payload.command.command.broadcastMessage.header.dataSize, (uint8_t*) temp, BROADCAST_CMD_HDR_MAX);

    memcpy(pkt.payload.command.command.broadcastMessage.payload, (uint8_t*) data, bytes);

    uint8_t pkt_size = (MODEM_COMMAND_PRE_MAX + MODEM_COMMAND_TYPE_MAX + BROADCAST_CMD_HDR_MAX + bytes);

#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Sending Broadcast Command Packet:\r\n");
    printBufferContents((uint8_t*) &pkt, pkt_size);
#endif // DEBUG_ON

    connection.write((uint8_t *) &pkt, pkt_size);
}

void
ping(
    HardwareSerial connection,
    uint8_t addr
){
    connection.printf("$P%03d", addr);
}

ParseResult
parse_status_query_packet(
    QueryStatusResponseFullPacket_t* statusResponse
){
    uint8_t node_addr = (uint8_t) fieldToInt((char*) statusResponse->addr, QUERY_STATUS_RESP_ADDR_MAX);

    uint16_t supply_voltage_meas = (uint16_t) fieldToInt((char*) statusResponse->voltPayload, QUERY_STATUS_RESP_VOLT_PAYLOAD_MAX);

    float supply_voltage = (float)(supply_voltage_meas) * 15.0f / 65536.0f;

#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Status query packet received.\r\n\tDevice addr : %03ld\r\n\tDevice Supply Voltage : %f\r\n", node_addr, supply_voltage);
#endif // DEBUG_ON

    ParseResult result;

    result.type = STATUS_QUERY_TYPE;
    result.status.node_addr = node_addr;
    result.status.voltage = supply_voltage;

    return result;
}

ParseResult
parse_set_address_packet(
    SetAddressResponsePacket_t* setAddressResponse
){
    uint8_t new_addr = (uint8_t) fieldToInt((char*) setAddressResponse->addr, SET_ADDRESS_RESP_ADDR_MAX);

#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Set address packet received.\r\n\tNew Device addr : %03ld\r\n", new_addr);
#endif // DEBUG_ON

    set_modem_id(new_addr);

    ParseResult result;

    result.type = SET_ADDR_TYPE;
    result.set_addr.new_addr = new_addr;

    return result;
}

ParseResult
parse_broadcast_packet(
    BroadcastMessageResponsePacket_t* broadcast
){
#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Parsing broadcast packet...\r\n");
#endif // DEBUG_ON

    uint8_t src_addr = fieldToInt((char*) broadcast->header.addr, BROADCAST_RESP_ADDR_MAX);
    uint8_t bytes = fieldToInt((char*) broadcast->header.dataSize, BROADCAST_RESP_DATA_SIZE_MAX);

    uint8_t* message = (uint8_t*) &broadcast->message;

#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Broadcast packet received.\r\n\tPacket Source Modem: %03ld\r\n\tMessage Size: %ld\r\n", src_addr, bytes);
#endif // DEBUG_ON

    ParseResult result;

    result.type = BROAD_RECV_TYPE;
    result.broadcast.src_addr = src_addr;
    result.broadcast.payload = message;
    result.broadcast.payload_size = bytes;

    return result;
}


// TODO: Fix up parse_unicast_packet
ParseResult
parse_unicast_packet(
    UnicastResponsePacket_t* unicast
){
    uint8_t bytes = (uint8_t) fieldToInt((char*) unicast->dataSize, UNICAST_RESP_DATA_SIZE_MAX);

    char message[bytes+1] = {0};
    memcpy(message, unicast->payload, bytes);

#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Unicast packet received.\r\n\tPacket Bytes : %ld\r\n\tPacket data : ", bytes);
    Serial.printf("%s\r\n", message);
#endif // DEBUG_ON
}

// Also handles ack packets from Unicast with ack command
ParseResult
parse_ping_packet(
    RangeDataResponsePacket_t* rangeResponse
){
    uint8_t src_addr = (uint8_t) fieldToInt((char*) rangeResponse->addr, RANGE_RESP_ADDR_MAX);
    uint16_t ping_propogation_counter = (uint16_t) fieldToInt((char*) rangeResponse->rangePayload, RANGE_RESP_PAYLOAD_MAX);

    float meter_range = static_cast<float>(ping_propogation_counter) * SOUND_SPEED * 3.125e-5;

#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Ping (or ACK) packet received.\r\n\tAddr : %03ld\r\n\tRange (m) : %f\r\n", src_addr, meter_range);
#endif // DEBUG_ON

    ParseResult result;

    result.type = PING_RESP_TYPE;
    result.ping.src_addr = src_addr;
    result.ping.meter_range = meter_range;

    neighborManager.update_neighbors(src_addr, meter_range);

    return result;
}

ParseResult
packet_received_modem(
    uint8_t* packetBuffer,
    uint8_t size
){
#ifdef DEBUG_ON // DEBUG_ON
    Serial.printf("Modem Packet Received...");
    printBufferContents((uint8_t*) packetBuffer, size);
#endif // DEBUG_ON

    if (size < 1) {
        // Should never happen over serial connection.
        return kNoneResult;
    }

    if (packetBuffer[0] == ERROR_TYPE && size == ERROR_MAX) { // 'E'
    #ifdef DEBUG_ON // DEBUG_ON
        Serial.printf("Error packet received.\r\n");
    #endif // DEBUG_ON

        ParseResult result;

        result.type = ERROR_RECV_TYPE;

        return result;
    }

    if (size < MODEM_MSG_MIN) {
    #ifdef DEBUG_ON // DEBUG_ON
        Serial.printf("Packet invalid, size too small.\r\n");
    #endif // DEBUG_ON

        return kNoneResult;
    }

    if (bloom_check_packet(packetBuffer, size)) {
    #ifdef DEBUG_ON
        Serial.printf("Duplicate packet (raw hash), dropping.\n");
    #endif
        return kNoneResult;
    }
        
    maybe_reset_bloom_filter();
    bloom_add_packet(packetBuffer, size);

    ModemPacket_t* pkt = (ModemPacket_t*) packetBuffer;

    if (pkt->type == MODEM_LOCAL_RESPONSE_TYPE) { // '$'
        ModemLocalResponsePacket_t* localResp = (ModemLocalResponsePacket_t* )&pkt->payload;

    #ifdef DEBUG_ON // DEBUG_ON
        Serial.printf("Local Echo...\r\n");
    #endif // DEBUG_ON

        // Local Echo
        switch (localResp->type) {
            case BROADCAST_CMD_LOCAL_RESP_TYPE: // 'B'
                if (size == BROADCAST_CMD_LOCAL_RESP_PACKET_SIZE) {
                #ifdef DEBUG_ON // DEBUG_ON
                    Serial.printf("Broadcast of ");

                    for (int i = 0; i < BROADCAST_CMD_LOCAL_RESP_DATA_SIZE_MAX; i++){
                        Serial.printf("%c", ((BroadcastLocalResponsePacket_t*) &localResp->response)->dataSize[i]);
                    }

                    Serial.printf(" bytes sent.\r\n");
                #endif // DEBUG_ON
                }
                break;
            case CHN_IMP_CMD_LOCAL_RESP_TYPE: // 'C' Will also be reached if it's a Corrected Error Local Response Packet
                break;
            case ECHO_MSG_CMD_LOCAL_RESP_TYPE: // 'E'
                break;
            case UNICAST_ACK_CMD_LOCAL_RESP_TYPE: // 'M'
                if (size == UNICAST_ACK_CMD_LOCAL_RESP_PACKET_SIZE) {
                    break;
                }
                break;
            case PING_CMD_LOCAL_RESP_TYPE: // 'P'
                if (size == PING_CMD_LOCAL_RESP_PACKET_SIZE) {
                #ifdef DEBUG_ON // DEBUG_ON
                    Serial.print("Ping to modem ");

                    for (int i = 0; i < PING_CMD_LOCAL_RESP_ADDR_MAX; i++) {
                        Serial.printf("%c", ((PingLocalResponsePacket_t*) &localResp->response)->addr[i]);
                    }

                    Serial.print("sent.\r\n");
                #endif // DEBUG_ON

                    break;
                }
                break;
            case RESET_CMD_LOCAL_RESP_TYPE: // 'R'
                break;
            case SPEC_MSR_CMD_LOCAL_RESP_TYPE: // 'S'
                break;
            case TEST_MSG_CMD_LOCAL_RESP_TYPE: // 'T'
                break;
            case UNICAST_CMD_LOCAL_RESP_TYPE: // 'U'
                if (size == UNICAST_CMD_LOCAL_RESP_PACKET_SIZE) {
                    break;
                }
                break;
            case VOLT_NOISE_MSR_CMD_LOCAL_RESP_TYPE: // 'V'
                break;
            default:
            #ifdef DEBUG_ON // DEBUG_ON
                Serial.printf("Unhandled packet type for local response.\r\n\tType Received: %c\r\n", (char) localResp->type);
            #endif // DEBUG_ON

                break;
        }
    } else if (pkt->type == MODEM_RESPONSE_TYPE) { // '#'
    #ifdef DEBUG_ON // DEBUG_ON
        Serial.printf("Response packet...\r\n");
    #endif // DEBUG_ON

        ModemResponsePacket_t* response = (ModemResponsePacket_t*)&pkt->payload;

        switch (response->type) {
            case QUERY_STATUS_RESP_TYPE: // 'A' Will also be reached if it's a Set Address Response Packet
            {
                QueryStatusResponsePacket_t* statusResponse = (QueryStatusResponsePacket_t*) &response->response;
                if (size == QUERY_STATUS_RESP_PACKET_SIZE) {
                    QueryStatusResponseFullPacket_t* fullStatus = (QueryStatusResponseFullPacket_t*) &statusResponse->status;
                    return parse_status_query_packet(fullStatus);
                } else if (size == SET_ADDRESS_RESP_PACKET_SIZE) {
                    SetAddressResponsePacket_t* setAddressResponse = (SetAddressResponsePacket_t*) &statusResponse->status;
                    return parse_set_address_packet(setAddressResponse);
                }
                break;
            }
            case BROADCAST_RESP_TYPE: // 'B'
            {
                BroadcastMessageResponsePacket_t* broadcast = (BroadcastMessageResponsePacket_t*) &response->response;
                return parse_broadcast_packet(broadcast);
                break;
            }
            case CHN_IMP_RESP_TYPE: // 'C'
                break;
            case RANGE_RESP_TYPE: // 'R'
            {
                RangeDataResponsePacket_t* rangeResponse = (RangeDataResponsePacket_t*) &response->response;

                if (size == RANGE_RESP_PACKET_SIZE) {
                    return parse_ping_packet(rangeResponse);
                }
                break;
            }
            case TIMEOUT_RESP_TYPE: // 'T'
                if (size == TIMEOUT_MAX) {
                #ifdef DEBUG_ON // DEBUG_ON
                    Serial.printf("Timeout.\r\n");
                #endif // DEBUG_ON
                }
                break;
            case UNICAST_RESP_TYPE: // 'U'
            {
                UnicastResponsePacket_t* unicast = (UnicastResponsePacket_t*) &response->response;
                return parse_unicast_packet(unicast);
                break;
            }

            default:
            #ifdef DEBUG_ON // DEBUG_ON
                Serial.printf("Unhandled packet type for modem response.\r\n\tType Received: %c\r\n", (char) response->type);
            #endif // DEBUG_ON

                break;
        }
    } else {
    #ifdef DEBUG_ON // DEBUG_ON
        Serial.printf("Error...\r\n");
        Serial.printf("Packet does not follow normal acoustic packet structure.\r\n\tPrefix: %c\r\n", (char) pkt->type);
    #endif // DEBUG_ON
    }

    return kNoneResult;
}