#pragma once

#include <stdint.h>
#include <string.h>

class RainbowBus {
    public:

    typedef struct {
        uint8_t dstId;      // 0 is master, 255 is broadcast
        uint8_t fromId;
        uint8_t length;     // payload length
        // uint8_t payload[];
        // uint16_t checksum;   // checksum at end of packet
    } rbbHeader_t;
    const int rbbHeaderSize = sizeof(rbbHeader_t);

    typedef struct {
        uint8_t packetType; // 0x00-0x10 is reserved for RainbowBus link layer
    } rbbPayload_t;

    enum class RbbPacketType {
        ACK = 0x01,
    };


    RainbowBus(void (*txFunc)(const uint8_t *buf, size_t size));
    void handleByte(uint8_t byte);
    void handleBytes(const uint8_t *bytes, size_t size);
    void sendPacket(uint8_t dstId, const uint8_t *payload, size_t payloadSize, bool needAck = false);
    // set txFunc
    // set callback
    // set id

    private:

    void (*_sendBytes)(const uint8_t *buf, size_t size);
    uint8_t _id;
    uint8_t _awaitingAckFromId = 0xFF;
};